/**
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#ifdef OGS_EMBED_PYTHON_INTERPRETER
#include "ogs_embedded_python.h"

#include <pybind11/embed.h>

#include "BaseLib/Logging.h"
#include "ProcessLib/BoundaryConditionAndSourceTerm/Python/BHEInflowPythonBoundaryConditionModule.h"
#include "ProcessLib/BoundaryConditionAndSourceTerm/Python/PythonBoundaryConditionModule.h"
#include "ProcessLib/BoundaryConditionAndSourceTerm/Python/PythonSourceTermModule.h"

PYBIND11_EMBEDDED_MODULE(OpenGeoSys, m)
{
    DBUG("Binding Python module OpenGeoSys.");

    ProcessLib::pythonBindBoundaryCondition(m);
    ProcessLib::bheInflowpythonBindBoundaryCondition(m);
    ProcessLib::SourceTerms::Python::pythonBindSourceTerm(m);
}

#ifndef OGS_BUILD_SHARED_LIBS

// Hackish trick that hopefully ensures that the linker won't strip the symbol
// pointed to by p from the library being built.
template <typename T>
void mark_used(T p)
{
    volatile T vp = p;
    vp = vp;
}

#endif  // OGS_BUILD_SHARED_LIBS

namespace ApplicationsLib
{
pybind11::scoped_interpreter setupEmbeddedPython()
{
#ifndef OGS_BUILD_SHARED_LIBS
    // pybind11_init_impl_OpenGeoSys is the function initializing the embedded
    // OpenGeoSys Python module. The name is generated by pybind11. If it is not
    // obvious that this symbol is actually used, the linker might remove it
    // under certain circumstances.
    mark_used(&pybind11_init_impl_OpenGeoSys);
#endif

    // Allows ogs to be interrupted by SIGINT, which otherwise is handled by
    // python. See
    // https://docs.python.org/3/c-api/exceptions.html#c.PyErr_CheckSignals and
    // https://pybind11.readthedocs.io/en/stable/faq.html#how-can-i-properly-handle-ctrl-c-in-long-running-functions
    constexpr bool init_signal_handlers = false;
    return pybind11::scoped_interpreter{init_signal_handlers};
}

}  // namespace ApplicationsLib
#endif
