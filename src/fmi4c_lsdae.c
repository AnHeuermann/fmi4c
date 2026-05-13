#include "fmi4c_lsdae.h"
#include "fmi4c_private.h"
#include "fmi4c_utils.h"
#include "fmi4c_common.h"
#include "ezxml/ezxml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LS_DAE_MANIFEST_RELPATH \
    "/extra/org.fmi-standard.fmi-ls-dae/fmi-ls-manifest.xml"

/* Parse a single ModelStructure entry element (ContinuousStateDerivative,
 * Residual, or Output) into an fmiLsDaeModelStructureHandle. */
static bool parseLsDaeStructureEntry(fmuHandle *fmu,
                                     fmiLsDaeModelStructureHandle *out,
                                     ezxml_t elem)
{
    parseUInt32AttributeEzXml(elem, "valueReference", &out->valueReference);

    out->numberOfDependencies  = 0;
    out->dependencyKindsDefined = false;
    out->dependencies           = NULL;
    out->dependencyKinds        = NULL;

    const char *depsStr = NULL;
    if (!parseStringAttributeEzXmlAndRememberPointer(elem, "dependencies", &depsStr, fmu)
        || depsStr == NULL || depsStr[0] == '\0') {
        return true; /* no dependencies attribute – that's fine */
    }

    /* Count space-separated tokens */
    char *mutable_deps = duplicateAndRememberString(fmu, depsStr);
    if (!mutable_deps) return false;

    int count = 1;
    for (int i = 0; mutable_deps[i]; ++i)
        if (mutable_deps[i] == ' ') ++count;

    out->numberOfDependencies = count;
    out->dependencies = mallocAndRememberPointer(fmu, count * sizeof(fmi3ValueReference));

    const char *delim = " ";
    for (int j = 0; j < count; ++j) {
        const char *tok = (j == 0) ? strtok(mutable_deps, delim) : strtok(NULL, delim);
        out->dependencies[j] = (fmi3ValueReference)strtoul(tok, NULL, 10);
    }

    /* Parse optional dependenciesKind */
    const char *kindsStr = NULL;
    if (!parseStringAttributeEzXmlAndRememberPointer(elem, "dependenciesKind", &kindsStr, fmu)
        || kindsStr == NULL || kindsStr[0] == '\0') {
        return true;
    }

    char *mutable_kinds = duplicateAndRememberString(fmu, kindsStr);
    if (!mutable_kinds) return false;

    out->dependencyKindsDefined = true;
    out->dependencyKinds = mallocAndRememberPointer(fmu, count * sizeof(fmi3DependencyKind));

    for (int j = 0; j < count; ++j) {
        const char *kind = (j == 0) ? strtok(mutable_kinds, delim) : strtok(NULL, delim);
        if      (!strcmp(kind, "independent")) out->dependencyKinds[j] = fmi3Independent;
        else if (!strcmp(kind, "constant"))    out->dependencyKinds[j] = fmi3Constant;
        else if (!strcmp(kind, "fixed"))       out->dependencyKinds[j] = fmi3Fixed;
        else if (!strcmp(kind, "tunable"))     out->dependencyKinds[j] = fmi3Tunable;
        else if (!strcmp(kind, "discrete"))    out->dependencyKinds[j] = fmi3Discrete;
        else                                   out->dependencyKinds[j] = fmi3Dependent;
    }
    return true;
}

/* Allow switch between schemes
 *
 * New schema:
 *
 * <Residual>
 *   <Formulation .../>
 * </Residual>
 *
 * Old schema:
 *
 * <Residual valueReference="..."/>
 */
static ezxml_t resolveStructureEntryElement(ezxml_t elem)
{
    ezxml_t formulation = ezxml_child(elem, "Formulation");

    if (formulation)
        return formulation;

    return elem;
}

/* Count child elements with a given name */
static int countChildren(ezxml_t parent, const char *name)
{
    int n = 0;

    for (ezxml_t e = ezxml_child(parent, name);
         e;
         e = ezxml_next(e))
    {
        ++n;
    }

    return n;
}

static void parseModelStructureArray(
    fmuHandle *fmu,
    ezxml_t parent,
    const char *tagName,
    fmiLsDaeModelStructureHandle **outArray,
    int *outCount)
{
    int n = countChildren(parent, tagName);

    *outCount = n;

    if (n == 0) {
        *outArray = NULL;
        return;
    }

    *outArray =
        mallocAndRememberPointer(fmu, n * sizeof(fmiLsDaeModelStructureHandle));

    int i = 0;

    for (ezxml_t e = ezxml_child(parent, tagName);
         e && i < n;
         e = ezxml_next(e), ++i)
    {
        ezxml_t actual = resolveStructureEntryElement(e);

        parseLsDaeStructureEntry(
            fmu,
            &(*outArray)[i],
            actual);
    }
}

/* Called from fmi4c_loadUnzippedFmu_internal after parseModelDescriptionFmi3.
 * Silently returns if the manifest file is absent. */
void parseFmiLsDaeManifest(fmuHandle *fmu)
{
    fmu->lsDae.isPresent = false;

    /* Build the absolute path to the manifest */
    char path[FILENAME_MAX];
    snprintf(path, sizeof(path), "%s" LS_DAE_MANIFEST_RELPATH, fmu->unzippedLocation);

    ezxml_t root = ezxml_parse_file(path);
    if (!root) return; /* manifest not present */

    if (strcmp(root->name, "fmiLayeredStandardManifest") != 0) {
        printf("[fmi-ls-dae] Unexpected root element: %s\n", root->name);
        ezxml_free(root);
        return;
    }

    fmu->lsDae.isPresent = true;

    /* Manifest attributes (namespace prefix is part of the literal name) */
    parseStringAttributeEzXmlAndRememberPointer(root, "fmi-ls:fmi-ls-name",
                                                &fmu->lsDae.fmiLsName, fmu);
    parseStringAttributeEzXmlAndRememberPointer(root, "fmi-ls:fmi-ls-version",
                                                &fmu->lsDae.fmiLsVersion, fmu);
    parseStringAttributeEzXmlAndRememberPointer(root, "fmi-ls:fmi-ls-description",
                                                &fmu->lsDae.fmiLsDescription, fmu);

    /* ---- AlgebraicVariables ---- */
    ezxml_t algVarsElem = ezxml_child(root, "AlgebraicVariables");
    if (algVarsElem) {
        int n = countChildren(algVarsElem, "AlgebraicVariable");
        fmu->lsDae.numberOfAlgebraicVariables = n;
        fmu->lsDae.algebraicVariables =
            mallocAndRememberPointer(fmu, n * sizeof(fmiLsDaeAlgebraicVariableHandle));
        int i = 0;
        for (ezxml_t e = ezxml_child(algVarsElem, "AlgebraicVariable"); e; e = ezxml_next(e), ++i)
            parseUInt32AttributeEzXml(e, "valueReference",
                                      &fmu->lsDae.algebraicVariables[i].valueReference);
    }

    /* ---- ModelStructure ---- */
    ezxml_t msElem = ezxml_child(root, "ModelStructure");
    if (!msElem) {
        ezxml_free(root);
        return;
    }

    /* ContinuousStateDerivative */
    parseModelStructureArray(
        fmu,
        msElem,
        "ContinuousStateDerivative",
        &fmu->lsDae.continuousStateDerivatives,
        &fmu->lsDae.numberOfContinuousStateDerivatives);

    /* Residual */
    parseModelStructureArray(
        fmu,
        msElem,
        "Residual",
        &fmu->lsDae.residuals,
        &fmu->lsDae.numberOfResiduals);

    /* Output */
    parseModelStructureArray(
        fmu,
        msElem,
        "Output",
        &fmu->lsDae.outputs,
        &fmu->lsDae.numberOfOutputs);
    ezxml_free(root);
}

/* ------------------------------------------------------------------ */
/* Public accessor implementations                                      */
/* ------------------------------------------------------------------ */

bool fmiLsDae_isPresent(fmuHandle *fmu)
{
    return fmu->lsDae.isPresent;
}

const char *fmiLsDae_getName(fmuHandle *fmu)
{
    return fmu->lsDae.fmiLsName;
}

const char *fmiLsDae_getVersion(fmuHandle *fmu)
{
    return fmu->lsDae.fmiLsVersion;
}

const char *fmiLsDae_getDescription(fmuHandle *fmu)
{
    return fmu->lsDae.fmiLsDescription;
}

/* AlgebraicVariables */

int fmiLsDae_getNumberOfAlgebraicVariables(fmuHandle *fmu)
{
    return fmu->lsDae.numberOfAlgebraicVariables;
}

fmiLsDaeAlgebraicVariableHandle *fmiLsDae_getAlgebraicVariableByIndex(fmuHandle *fmu, int i)
{
    if (i < 0 || i >= fmu->lsDae.numberOfAlgebraicVariables) return NULL;
    return &fmu->lsDae.algebraicVariables[i];
}

fmi3ValueReference fmiLsDae_getAlgebraicVariableValueReference(fmiLsDaeAlgebraicVariableHandle *var)
{
    return var->valueReference;
}

/* ContinuousStateDerivative */

int fmiLsDae_getNumberOfContinuousStateDerivatives(fmuHandle *fmu)
{
    return fmu->lsDae.numberOfContinuousStateDerivatives;
}

fmiLsDaeModelStructureHandle *fmiLsDae_getContinuousStateDerivativeByIndex(fmuHandle *fmu, int i)
{
    if (i < 0 || i >= fmu->lsDae.numberOfContinuousStateDerivatives) return NULL;
    return &fmu->lsDae.continuousStateDerivatives[i];
}

/* Residual */

int fmiLsDae_getNumberOfResiduals(fmuHandle *fmu)
{
    return fmu->lsDae.numberOfResiduals;
}

fmiLsDaeModelStructureHandle *fmiLsDae_getResidualByIndex(fmuHandle *fmu, int i)
{
    if (i < 0 || i >= fmu->lsDae.numberOfResiduals) return NULL;
    return &fmu->lsDae.residuals[i];
}

/* Output */

int fmiLsDae_getNumberOfOutputs(fmuHandle *fmu)
{
    return fmu->lsDae.numberOfOutputs;
}

fmiLsDaeModelStructureHandle *fmiLsDae_getOutputByIndex(fmuHandle *fmu, int i)
{
    if (i < 0 || i >= fmu->lsDae.numberOfOutputs) return NULL;
    return &fmu->lsDae.outputs[i];
}

/* ModelStructure handle accessors */

fmi3ValueReference fmiLsDae_getValueReference(fmiLsDaeModelStructureHandle *h)
{
    return h->valueReference;
}

int fmiLsDae_getNumberOfDependencies(fmiLsDaeModelStructureHandle *h)
{
    return h->numberOfDependencies;
}

bool fmiLsDae_dependencyKindsDefined(fmiLsDaeModelStructureHandle *h)
{
    return h->dependencyKindsDefined;
}

void fmiLsDae_getDependencies(fmiLsDaeModelStructureHandle *h,
                               fmi3ValueReference *deps, int n)
{
    int count = (n < h->numberOfDependencies) ? n : h->numberOfDependencies;
    for (int i = 0; i < count; ++i)
        deps[i] = h->dependencies[i];
}

void fmiLsDae_getDependencyKinds(fmiLsDaeModelStructureHandle *h,
                                  fmi3DependencyKind *kinds, int n)
{
    if (!h->dependencyKindsDefined) return;
    int count = (n < h->numberOfDependencies) ? n : h->numberOfDependencies;
    for (int i = 0; i < count; ++i)
        kinds[i] = h->dependencyKinds[i];
}
