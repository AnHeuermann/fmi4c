#ifndef FMI4C_LSDAE_H
#define FMI4C_LSDAE_H

/*!
 * @file fmi4c_lsdae.h
 * @brief fmi4c extension for the fmi-ls-dae layered standard.
 *
 * The fmi-ls-dae layered standard augments an FMI 3.0 FMU with DAE
 * information stored in
 * @c extra/org.fmi-standard.fmi-ls-dae/fmi-ls-manifest.xml.
 *
 * This extension parses that manifest automatically when the FMU is
 * loaded via fmi4c_loadFmu() / fmi4c_loadUnzippedFmu() and exposes
 * the data through the @c fmiLsDae_* accessor functions, mirroring
 * the @c fmi3_* accessor pattern used for modelDescription.xml data.
 *
 * The manifest is optional. Always call fmiLsDae_isPresent() before
 * any other accessor to verify that the FMU carries the extension.
 *
 * @see https://github.com/modelica/fmi-ls-dae
 */

#include "fmi4c_types_fmi3.h"   /* fmi3ValueReference, fmi3DependencyKind */
#include "fmi4c_common.h"

#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Public handle types                                                  */
/* ------------------------------------------------------------------ */

/*!
 * @brief Handle for one entry in the @c AlgebraicVariables list.
 *
 * Obtain instances via fmiLsDae_getAlgebraicVariableByIndex().
 */
typedef struct {
    fmi3ValueReference valueReference; /*!< Value reference of the algebraic variable */
} fmiLsDaeAlgebraicVariableHandle;

/*!
 * @brief Handle for one entry in the fmi-ls-dae @c ModelStructure.
 *
 * Used for @c ContinuousStateDerivative, @c Residual and @c Output
 * entries.  Obtain instances via the corresponding
 * fmiLsDae_get*ByIndex() functions.
 */
typedef struct {
    fmi3ValueReference  valueReference;       /*!< Value reference of the entry */
    int                 numberOfDependencies; /*!< Number of entries in @c dependencies */
    bool                dependencyKindsDefined; /*!< True when @c dependencyKinds is valid */
    fmi3ValueReference *dependencies;    /*!< Array of dependency value references [numberOfDependencies] */
    fmi3DependencyKind *dependencyKinds; /*!< Array of dependency kinds [numberOfDependencies]; valid only when dependencyKindsDefined is true */
} fmiLsDaeModelStructureHandle;

/* ------------------------------------------------------------------ */
/* DLL-export macro                                                     */
/* ------------------------------------------------------------------ */

/* When building as a static library (FMI4C_STATIC), FMI4C_DLLAPI
 * expands to nothing. */
#ifdef FMI4C_STATIC
#define FMI4C_DLLAPI
#elif defined(FMI4C_DLLEXPORT)
#ifdef _WIN32
#define FMI4C_DLLAPI __declspec(dllexport)
#else
#define FMI4C_DLLAPI __attribute__((visibility("default")))
#endif
#else
#ifdef _WIN32
#define FMI4C_DLLAPI __declspec(dllimport)
#else
#define FMI4C_DLLAPI
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Presence check                                                       */
/* ------------------------------------------------------------------ */

//! @brief Returns whether the FMU contains a fmi-ls-dae manifest.
//! @param fmu FMU handle
//! @returns True if extra/org.fmi-standard.fmi-ls-dae/fmi-ls-manifest.xml was found and parsed successfully
FMI4C_DLLAPI bool fmiLsDae_isPresent(fmuHandle *fmu);

/* ------------------------------------------------------------------ */
/* Manifest metadata                                                    */
/* ------------------------------------------------------------------ */

//! @brief Returns the layered-standard name from the manifest (fmi-ls:fmi-ls-name attribute).
//! @param fmu FMU handle
//! @returns Layered standard name string, or NULL if the manifest is absent
FMI4C_DLLAPI const char *fmiLsDae_getName(fmuHandle *fmu);

//! @brief Returns the layered-standard version from the manifest (fmi-ls:fmi-ls-version attribute).
//! @param fmu FMU handle
//! @returns Layered standard version string, or NULL if the manifest is absent
FMI4C_DLLAPI const char *fmiLsDae_getVersion(fmuHandle *fmu);

//! @brief Returns the layered-standard description from the manifest (fmi-ls:fmi-ls-description attribute).
//! @param fmu FMU handle
//! @returns Layered standard description string, or NULL if the manifest is absent
FMI4C_DLLAPI const char *fmiLsDae_getDescription(fmuHandle *fmu);

/* ------------------------------------------------------------------ */
/* AlgebraicVariables                                                   */
/* ------------------------------------------------------------------ */

//! @brief Returns the number of algebraic variables declared in the manifest.
//! @param fmu FMU handle
//! @returns Number of AlgebraicVariable entries
FMI4C_DLLAPI int fmiLsDae_getNumberOfAlgebraicVariables(fmuHandle *fmu);

//! @brief Returns the algebraic variable at the given 0-based index.
//! @param fmu FMU handle
//! @param i   0-based index in [0, fmiLsDae_getNumberOfAlgebraicVariables()-1]
//! @returns Handle to the algebraic variable, or NULL if index is out of range
FMI4C_DLLAPI fmiLsDaeAlgebraicVariableHandle *fmiLsDae_getAlgebraicVariableByIndex(fmuHandle *fmu, int i);

//! @brief Returns the value reference of an algebraic variable.
//! @param var Algebraic variable handle
//! @returns Value reference
FMI4C_DLLAPI fmi3ValueReference fmiLsDae_getAlgebraicVariableValueReference(fmiLsDaeAlgebraicVariableHandle *var);

/* ------------------------------------------------------------------ */
/* ModelStructure – ContinuousStateDerivative                           */
/* ------------------------------------------------------------------ */

//! @brief Returns the number of ContinuousStateDerivative entries in the manifest ModelStructure.
//! @param fmu FMU handle
//! @returns Number of ContinuousStateDerivative entries
FMI4C_DLLAPI int fmiLsDae_getNumberOfContinuousStateDerivatives(fmuHandle *fmu);

//! @brief Returns the ContinuousStateDerivative entry at the given 0-based index.
//! @param fmu FMU handle
//! @param i   0-based index in [0, fmiLsDae_getNumberOfContinuousStateDerivatives()-1]
//! @returns Handle to the entry, or NULL if index is out of range
FMI4C_DLLAPI fmiLsDaeModelStructureHandle *fmiLsDae_getContinuousStateDerivativeByIndex(fmuHandle *fmu, int i);

/* ------------------------------------------------------------------ */
/* ModelStructure – Residual                                            */
/* ------------------------------------------------------------------ */

//! @brief Returns the number of Residual entries in the manifest ModelStructure.
//! @param fmu FMU handle
//! @returns Number of Residual entries
FMI4C_DLLAPI int fmiLsDae_getNumberOfResiduals(fmuHandle *fmu);

//! @brief Returns the Residual entry at the given 0-based index.
//! @param fmu FMU handle
//! @param i   0-based index in [0, fmiLsDae_getNumberOfResiduals()-1]
//! @returns Handle to the entry, or NULL if index is out of range
FMI4C_DLLAPI fmiLsDaeModelStructureHandle *fmiLsDae_getResidualByIndex(fmuHandle *fmu, int i);

/* ------------------------------------------------------------------ */
/* ModelStructure – Output                                              */
/* ------------------------------------------------------------------ */

//! @brief Returns the number of Output entries in the manifest ModelStructure.
//! @param fmu FMU handle
//! @returns Number of Output entries
FMI4C_DLLAPI int fmiLsDae_getNumberOfOutputs(fmuHandle *fmu);

//! @brief Returns the Output entry at the given 0-based index.
//! @param fmu FMU handle
//! @param i   0-based index in [0, fmiLsDae_getNumberOfOutputs()-1]
//! @returns Handle to the entry, or NULL if index is out of range
FMI4C_DLLAPI fmiLsDaeModelStructureHandle *fmiLsDae_getOutputByIndex(fmuHandle *fmu, int i);

/* ------------------------------------------------------------------ */
/* ModelStructure handle accessors (shared by all three entry types)   */
/* ------------------------------------------------------------------ */

//! @brief Returns the value reference of a model-structure entry.
//! @param h Model-structure entry handle
//! @returns Value reference
FMI4C_DLLAPI fmi3ValueReference fmiLsDae_getValueReference(fmiLsDaeModelStructureHandle *h);

//! @brief Returns the number of dependencies of a model-structure entry.
//! @param h Model-structure entry handle
//! @returns Number of dependency value references
FMI4C_DLLAPI int fmiLsDae_getNumberOfDependencies(fmiLsDaeModelStructureHandle *h);

//! @brief Returns whether the dependenciesKind attribute was present for this entry.
//! @param h Model-structure entry handle
//! @returns True if dependency kinds are defined and fmiLsDae_getDependencyKinds() is meaningful
FMI4C_DLLAPI bool fmiLsDae_dependencyKindsDefined(fmiLsDaeModelStructureHandle *h);

//! @brief Copies dependency value references into a caller-provided array.
//! @param h    Model-structure entry handle
//! @param deps Output array; must have room for at least @p n elements
//! @param n    Maximum number of entries to copy; typically fmiLsDae_getNumberOfDependencies()
FMI4C_DLLAPI void fmiLsDae_getDependencies(fmiLsDaeModelStructureHandle *h,
                                            fmi3ValueReference *deps, int n);

//! @brief Copies dependency kinds into a caller-provided array.
//! @param h     Model-structure entry handle
//! @param kinds Output array; must have room for at least @p n elements
//! @param n     Maximum number of entries to copy; typically fmiLsDae_getNumberOfDependencies()
//! @note Does nothing if fmiLsDae_dependencyKindsDefined() returns false
FMI4C_DLLAPI void fmiLsDae_getDependencyKinds(fmiLsDaeModelStructureHandle *h,
                                               fmi3DependencyKind *kinds, int n);

#ifdef __cplusplus
}
#endif

#endif /* FMI4C_LSDAE_H */
