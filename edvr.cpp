// This example is for MSVC compilation only
// No attempt has been made to make it cross platform or work with other compilers

#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <type_traits>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef _WIN64
#pragma pack(push, 1)
#pragma warning(disable : 4103)
#endif

// LabVIEW Manager Functions Error Type
using LV_MgErr_t = int32_t;

// Error Codes
// https://www.ni.com/docs/en-US/bundle/labview/page/labview-manager-function-errors.html
const LV_MgErr_t LV_ERR_noError = 0;
const LV_MgErr_t LV_ERR_bogusError = 42;

// Generic LabVIEW Refs type
using LV_MagicCookie_t = uint32_t;

// LabVIEW EDVR structure types

// a dimension specifier used to specify the size of this dimension
// and the stride (how many elements to increment a pointer by to get to the next element in this direction)
struct LV_EDVRDimensionSpecifier_t
{
    size_t dimension_size;
    ptrdiff_t stride;
};

// forward decleration of type for use in callback functions
struct LV_EDVRData_t;
using LV_EDVRDataPtr_t = LV_EDVRData_t*;

//  callback function definitions
using LV_EDVROnDeleteCallbackFnPtr_t = std::add_pointer_t<void(LV_EDVRDataPtr_t)>;
using LV_EDVROnLockChangeCallbackFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRDataPtr_t)>;

// EDVR Data
struct LV_EDVRData_t
{
    uintptr_t metadata_ptr;
    int32_t n_dims;
    uintptr_t *data_ptr;
    LV_EDVRDimensionSpecifier_t dimension_specifier[5];
    LV_EDVROnDeleteCallbackFnPtr_t delete_callback_fn_ptr;
    LV_EDVROnLockChangeCallbackFnPtr_t lock_callback_fn_ptr;
    LV_EDVROnLockChangeCallbackFnPtr_t unlock_callback_fn_ptr;
};

#ifndef _WIN64
#pragma pack(pop)
#endif

// EDVR Function Pointers - see ni_extcode.h included in examples with the KB article
// "Customizing GPU Computing Using the LabVIEW GPU Analysis Toolkit"
// https://knowledge.ni.com/KnowledgeArticleDetails?id=kA00Z0000015AcdSAE&l=en-GB

using LV_EDVRContext_t = LV_MagicCookie_t;
using LV_EDVRReference_t = LV_MagicCookie_t;
using LV_EDVRReferencePtr_t = LV_EDVRReference_t*;
using LV_EDVRDataHandle_t = LV_EDVRData_t**;

// Define Function Pointer Types
// MgErr EDVR_GetCurrentContext(ExternalDataValueReferenceContext* pContext);
using LV_EDVRGetCurrentContextFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRContext_t*)>;
// MgErr EDVR_CreateReference(ExternalDataValueReference* pReference, ExternalDataValueReferenceData** ppReferenceData);
using LV_EDVRCreateReferenceFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRReferencePtr_t, LV_EDVRDataHandle_t)>;
// MgErr EDVR_AddRefWithContext(ExternalDataValueReference reference, ExternalDataValueReferenceContext context, ExternalDataValueReferenceData** ppReferenceData);
using LV_EDVRAddRefWithContextFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRReference_t, LV_EDVRContext_t, LV_EDVRDataHandle_t)>;
// MgErr EDVR_ReleaseRefWithContext(ExternalDataValueReference reference, ExternalDataValueReferenceContext context);
using LV_EDVRReleaseRefWithContextFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRReference_t, LV_EDVRContext_t)>;

// Imported function pointers (will be set in DLL_PROCESS_ATTACH)
static LV_EDVRGetCurrentContextFnPtr_t EDVR_GetCurrentContextImp = nullptr;
static LV_EDVRCreateReferenceFnPtr_t EDVR_CreateReferenceImp = nullptr;
static LV_EDVRAddRefWithContextFnPtr_t EDVR_AddRefWithContextImp = nullptr;
static LV_EDVRReleaseRefWithContextFnPtr_t EDVR_ReleaseRefWithContextImp = nullptr;

// Declare functions which call the imported LabVIEW runtime functions
LV_MgErr_t EDVR_GetCurrentContext(LV_EDVRContext_t* ctx_ptr)
{
    return EDVR_GetCurrentContextImp ? EDVR_GetCurrentContextImp(ctx_ptr) : LV_ERR_bogusError;
}
LV_MgErr_t EDVR_CreateReference(LV_EDVRReferencePtr_t edvr_ref_ptr, LV_EDVRDataHandle_t hndl)
{
    return EDVR_CreateReferenceImp ? EDVR_CreateReferenceImp(edvr_ref_ptr, hndl) : LV_ERR_bogusError;
}
LV_MgErr_t EDVR_AddRefWithContext(LV_EDVRReference_t edvr_ref_ptr, LV_EDVRContext_t ctx, LV_EDVRDataHandle_t hndl)
{
    return EDVR_AddRefWithContextImp ? EDVR_AddRefWithContextImp(edvr_ref_ptr, ctx, hndl) : LV_ERR_bogusError;
}
LV_MgErr_t EDVR_ReleaseRefWithContext(LV_EDVRReference_t edvr_ref_ptr, LV_EDVRContext_t ctx)
{
    return EDVR_ReleaseRefWithContextImp ? EDVR_ReleaseRefWithContextImp(edvr_ref_ptr, ctx) : LV_ERR_bogusError;
}

// use LabVIEW's DbgPrintf as well for debug printing
using LV_DbgPrintfFnPtr_t = std::add_pointer_t<LV_MgErr_t(const char *, ...)>;
static LV_DbgPrintfFnPtr_t DbgPrintfImp = nullptr;

LV_MgErr_t DbgPrintf(const char *fmt, ...)
{
    if (DbgPrintfImp == nullptr)
    {
        return LV_ERR_bogusError;
    }
    va_list args;
    va_start(args, fmt);
    DbgPrintfImp(fmt, args);
    va_end(args);
    return LV_ERR_noError;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {

        HMODULE module = nullptr;

        // try loading from the normal runtimes
        static const char *const runtimes[] = {"LabVIEW.exe", "lvffrt.dll", "lvrt.dll"};

        // work through the list of runtimes
        for (const auto &runtime : runtimes)
        {
            module = GetModuleHandleA(runtime);
            if (module)
                break;
        }

        if (!module)
        {
            return false;
        }

        EDVR_GetCurrentContextImp = reinterpret_cast<LV_EDVRGetCurrentContextFnPtr_t>(GetProcAddress(module, "EDVR_GetCurrentContext"));
        EDVR_CreateReferenceImp = reinterpret_cast<LV_EDVRCreateReferenceFnPtr_t>(GetProcAddress(module, "EDVR_CreateReference"));
        EDVR_AddRefWithContextImp = reinterpret_cast<LV_EDVRAddRefWithContextFnPtr_t>(GetProcAddress(module, "EDVR_AddRefWithContext"));
        EDVR_ReleaseRefWithContextImp = reinterpret_cast<LV_EDVRReleaseRefWithContextFnPtr_t>(GetProcAddress(module, "EDVR_ReleaseRefWithContext"));

        // DbgPrintf
        DbgPrintfImp = reinterpret_cast<LV_DbgPrintfFnPtr_t>(GetProcAddress(module, "DbgPrintf"));
    }
    break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return true;
}

/**********************************************************************************************************/
//
//      Example Usage - Use a std::vector to mimic an object which exposes memory to LabVIEW
//
/**********************************************************************************************************/

// Using a std::vector as our object is fine BUT we don't have any protection on parallel access from either C++ or LabVIEW!
// This means me must be very careful in our labVIEW code to prevent parallel calls of the C++ code AND avoid calling any C++ calls whilst LabVIEW has access to the data

// Guaranteeing synchronized access on the C++ side by making an object which holds the std::vector and a std::mutex, std::condition_variable and
// a variable holding the lock-owner (NONE, LABVIEW, CPP, CPP_READ_ONLY etc) would probably be a better approach to improve memory safety

#include <vector>

// define the EDVR callbacks
LV_MgErr_t on_labview_lock(LV_EDVRDataPtr_t data_ptr)
{
    // this is where we would block until any C++ operations had completed
    // or return an error if we want to signal that LabVIEW cannot have the data - like because it already has a lock on it!
    // We could also ensure that the edvr_data_ptr's data_ptr, n_dims or dimension_specifier are up to date here if we don't elsewhere
    
    DbgPrintf("Granting access to the data to LabVIEW.");
    return LV_ERR_noError;
}

LV_MgErr_t on_labview_unlock(LV_EDVRDataPtr_t data_ptr)
{
    // this is where we would record that LabVIEW has finished with the data
    // the size cannot be changed but the values might have been modified by LabVIEW
    DbgPrintf("LabVIEW relinquished access to the data.");
    return LV_ERR_noError;
}

void on_labview_delete(LV_EDVRDataPtr_t data_ptr)
{

    DbgPrintf("LabVIEW signalling the EDVR is being deleted.");

    // clean up std::vector
    auto v = reinterpret_cast<std::vector<double> *>(data_ptr->metadata_ptr);
    delete v;

    DbgPrintf("Cleanup of internal data done.");
}

extern "C"
{

    __declspec(dllexport) LV_MgErr_t example_create_object(LV_EDVRReferencePtr_t ref_ptr)
    {
        LV_MgErr_t result = LV_ERR_noError;

        // allocate EDVR Data Structure
        LV_EDVRDataPtr_t edvr_data_ptr = nullptr;
        result = EDVR_CreateReference(ref_ptr, &edvr_data_ptr);

        // we want to heap allocate a vector so its lifetime extends beyond this function call
        auto v = new std::vector<double>;

        // use vector as a buffer but representing an 2D array
        const size_t rows = 2;
        const size_t cols = 5;

        const size_t size{rows * cols};

        v->reserve(size);
        for (int i=0; i < size; i++)
        {
            v->push_back(i + 0.5f);
        }

        // Initialize values in EDVR Data Structure

        // set metadata_ptr to the std::vector pointer
        // this will allow us to delete the vector in the EDVR on_delete callback
        edvr_data_ptr->metadata_ptr = reinterpret_cast<uintptr_t>(v);

        // set the data_ptr to the start of the vector's internal data
        edvr_data_ptr->data_ptr = reinterpret_cast<uintptr_t *>(v->data());
        edvr_data_ptr->n_dims = 2; // 2 dimension array
        // specify sizes in element and the strides in bytes
        edvr_data_ptr->dimension_specifier[0] = {rows, static_cast<ptrdiff_t>(rows * sizeof(double))};
        edvr_data_ptr->dimension_specifier[1] = {cols, static_cast<ptrdiff_t>(sizeof(double))};
        // set callbacks
        edvr_data_ptr->lock_callback_fn_ptr = on_labview_lock;
        edvr_data_ptr->unlock_callback_fn_ptr = on_labview_unlock;
        edvr_data_ptr->delete_callback_fn_ptr = on_labview_delete;

        return LV_ERR_noError;
    }

    __declspec(dllexport) LV_MgErr_t example_triple_values(LV_EDVRReferencePtr_t ref_ptr)
    {

        // Lets hope we have exclusive access!!

        LV_MgErr_t err = LV_ERR_noError;

        // Get EDVR Context
        LV_EDVRContext_t ctx;
        err = EDVR_GetCurrentContext(&ctx);

        if (err != LV_ERR_noError)
        {
            DbgPrintf(">> Failed to get context for EDVR operations!");
            return err;
        }

        // Access the EDVR-Data referenced by the EDVR-refnum

        LV_EDVRDataPtr_t edvr_data_ptr;
        err = EDVR_AddRefWithContext(*ref_ptr, ctx, &edvr_data_ptr);

        if (err != LV_ERR_noError)
        {
            DbgPrintf(">> Failed to get EDVR Data from EDVR_Reference!");
            return err;
        }

        auto v = reinterpret_cast<std::vector<double> *>(edvr_data_ptr->metadata_ptr);

        for (auto &value : *v)
        {
            value = value * 3;
        }

        // Data size/memory-backing is unchanged so we don't need to update edvr_data_ptr's data_ptr, n_dims or dimension_specifier

        // Finished with the EDVR-Data
        err = EDVR_ReleaseRefWithContext(*ref_ptr, ctx);

        return LV_ERR_noError;
    }
}
