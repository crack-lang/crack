// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

// Note: this code was mostly lifted from the LLVM ExceptionDemo.cpp file.

#include "BorrowedExceptions.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>
#include "ItaniumExceptionABI.h"
#include "Exceptions.h"

#define DW_EH_PE_omit	0xff
#define DW_EH_PE_uleb128	0x01
#define DW_EH_PE_udata2	0x02
#define DW_EH_PE_udata4	0x03
#define DW_EH_PE_udata8	0x04
#define DW_EH_PE_sleb128	0x09
#define DW_EH_PE_sdata2	0x0A
#define DW_EH_PE_sdata4	0x0B
#define DW_EH_PE_sdata8	0x0C
#define DW_EH_PE_absptr	0x00
#define DW_EH_PE_pcrel	0x10
#define DW_EH_PE_textrel 0x20
#define DW_EH_PE_datarel	0x30
#define DW_EH_PE_funcrel	0x40
#define DW_EH_PE_aligned	0x50
#define DW_EH_PE_indirect	0x80
#define DW_EH_PE_omit	0xff

using std::cerr;
using std::endl;

namespace crack { namespace runtime {

/// Read a uleb128 encoded value and advance pointer 
/// See Variable Length Data in: 
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// @param data reference variable holding memory pointer to decode from
/// @returns decoded value
static uintptr_t readULEB128(const uint8_t** data) {
    uintptr_t result = 0;
    uintptr_t shift = 0;
    unsigned char byte;
    const uint8_t* p = *data;

    do {
        byte = *p++;
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } 
    while (byte & 0x80);

    *data = p;

    return result;
}


/// Read a sleb128 encoded value and advance pointer 
/// See Variable Length Data in: 
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// @param data reference variable holding memory pointer to decode from
/// @returns decoded value
static uintptr_t readSLEB128(const uint8_t** data) {
    uintptr_t result = 0;
    uintptr_t shift = 0;
    unsigned char byte;
    const uint8_t* p = *data;

    do {
        byte = *p++;
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } 
    while (byte & 0x80);

    *data = p;

    if ((byte & 0x40) && (shift < (sizeof(result) << 3))) {
        result |= (~0 << shift);
    }

    return result;
}


/// Read a pointer encoded value and advance pointer 
/// See Variable Length Data in: 
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// @param data reference variable holding memory pointer to decode from
/// @param encoding dwarf encoding type
/// @returns decoded value
static uintptr_t readEncodedPointer(const uint8_t** data, uint8_t encoding) {
    uintptr_t result = 0;
    const uint8_t* p = *data;

    if (encoding == DW_EH_PE_omit) 
        return(result);

    // first get value 
    switch (encoding & 0x0F) {
        case DW_EH_PE_absptr:
            result = *((uintptr_t*)p);
            p += sizeof(uintptr_t);
            break;
        case DW_EH_PE_uleb128:
            result = readULEB128(&p);
            break;
        // Note: This case has not been tested
        case DW_EH_PE_sleb128:
            result = readSLEB128(&p);
            break;
        case DW_EH_PE_udata2:
            result = *((uint16_t*)p);
            p += sizeof(uint16_t);
            break;
        case DW_EH_PE_udata4:
            result = *((uint32_t*)p);
            p += sizeof(uint32_t);
            break;
        case DW_EH_PE_udata8:
            result = *((uint64_t*)p);
            p += sizeof(uint64_t);
            break;
        case DW_EH_PE_sdata2:
            result = *((int16_t*)p);
            p += sizeof(int16_t);
            break;
        case DW_EH_PE_sdata4:
            result = *((int32_t*)p);
            p += sizeof(int32_t);
            break;
        case DW_EH_PE_sdata8:
            result = *((int64_t*)p);
            p += sizeof(int64_t);
            break;
        default:
            // not supported 
            abort();
            break;
    }

    // then add relative offset 
    switch (encoding & 0x70) {
        case DW_EH_PE_absptr:
            // do nothing 
            break;
        case DW_EH_PE_pcrel:
            result += (uintptr_t)(*data);
            break;
        case DW_EH_PE_textrel:
        case DW_EH_PE_datarel:
        case DW_EH_PE_funcrel:
        case DW_EH_PE_aligned:
        default:
            // not supported 
            abort();
            break;
    }

    // then apply indirection 
    if (encoding & DW_EH_PE_indirect) {
        result = *((uintptr_t*)result);
    }

    *data = p;

    return result;
}

static void *getTTypePtr(void **classInfo, int index, uint8_t ttypeEncoding) {
    switch (ttypeEncoding & 0xF) {
        case DW_EH_PE_absptr:
            return classInfo[index];
        case DW_EH_PE_udata4:
            return (void *)((uint32_t *)classInfo)[index];
        case DW_EH_PE_udata8:
            return (void *)((uint64_t *)classInfo)[index];
        default:
            cerr << "Unexpected type pointer encoding type: " << 
                ttypeEncoding << endl;
            abort();
    }
}

/// Deals with Dwarf actions matching our type infos. 
/// Returns whether or not a dwarf emitted 
/// action matches the supplied exception type. If such a match succeeds, 
/// the resultAction argument will be set with > 0 index value. Only 
/// corresponding llvm.eh.selector type info arguments, cleanup arguments 
/// are supported. Filters are not supported.
/// See Variable Length Data in: 
/// @link http://dwarfstd.org/Dwarf3.pdf @unlink
/// Also see @link http://refspecs.freestandards.org/abi-eh-1.21.html @unlink
/// @param resultAction reference variable which will be set with result
/// @param classInfo our array of type info pointers (to globals)
/// @param actionEntry index into above type info array or 0 (clean up). 
///        We do not support filters.
/// @param exceptionClass exception class (_Unwind_Exception::exception_class)
///        of thrown exception.
/// @param exceptionObject thrown _Unwind_Exception instance.
/// @returns whether or not a type info was found. False is returned if only
///          a cleanup was found
static bool handleActionValue(int64_t *resultAction,
                              uint8_t ttypeEncoding,
                              void **classInfo, 
                              uintptr_t actionEntry, 
                              uint64_t exceptionClass, 
                              struct _Unwind_Exception *exceptionObject) {
    bool ret = false;

    if (!resultAction || 
        !exceptionObject || 
        (exceptionClass != crackClassId))
        return(ret);

    // get the "crack exception object" which is the object actually thrown in 
    // the "throw" statement.
    void *crackExceptionObject = exceptionObject->user_data;

#ifdef DEBUG
    fprintf(stderr,
            "handleActionValue(...): exceptionObject = <%p>, "
                "excp = <%p>.\n",
            exceptionObject,
            excp);
#endif

    const uint8_t *actionPos = (uint8_t*) actionEntry,
                  *tempActionPos;
    int64_t typeOffset = 0,
            actionOffset;

    for (int i = 0; true; ++i) {
        // Each emitted dwarf action corresponds to a 2 tuple of
        // type info address offset, and action offset to the next
        // emitted action.
        typeOffset = readSLEB128(&actionPos);
        tempActionPos = actionPos;
        actionOffset = readSLEB128(&tempActionPos);

#ifdef DEBUG
        fprintf(stderr,
                "handleActionValue(...):typeOffset: <%lld>, "
                    "actionOffset: <%lld>.\n",
                typeOffset,
                actionOffset);
#endif
        assert((typeOffset >= 0) && 
               "handleActionValue(...):filters are not supported.");

        // Note: A typeOffset == 0 implies that a cleanup llvm.eh.selector
        //       argument has been matched.
        if (typeOffset > 0) {
            // if we got an exception and there is no "match" function, 
            // translate it to an abort.
            if (!runtimeHooks.exceptionMatchFunc) abort();
            void *curClassType = getTTypePtr(classInfo, -typeOffset, 
                                             ttypeEncoding
                                             );
            if (runtimeHooks.exceptionMatchFunc(curClassType, 
                                                crackExceptionObject
                                                )
                ) {
#ifdef DEBUG
                fprintf(stderr,
                        "handleActionValue(...):actionValue <%d> found.\n",
                        i);
#endif
                *resultAction = i + 1;
                ret = true;
                break;
            }
        }

#ifdef DEBUG
        fprintf(stderr,
                "handleActionValue(...):actionValue not found.\n");
#endif
        if (!actionOffset)
            break;

        actionPos += actionOffset;
    }

    return(ret);
}

/// Deals with the Language specific data portion of the emitted dwarf code.
/// See @link http://refspecs.freestandards.org/abi-eh-1.21.html @unlink
/// @param version unsupported (ignored), unwind version
/// @param lsda language specific data area
/// @param _Unwind_Action actions minimally supported unwind stage 
///        (forced specifically not supported)
/// @param exceptionClass exception class (_Unwind_Exception::exception_class)
///        of thrown exception.
/// @param exceptionObject thrown _Unwind_Exception instance.
/// @param context unwind system context
/// @returns minimally supported unwinding control indicator 
_Unwind_Reason_Code handleLsda(int version, 
                               const uint8_t* lsda,
                               _Unwind_Action actions,
                               uint64_t exceptionClass, 
                               struct _Unwind_Exception* exceptionObject,
                               _Unwind_Context *context) {
    _Unwind_Reason_Code ret = _URC_CONTINUE_UNWIND;

    if (!lsda)
        return(ret);

#ifdef DEBUG
    fprintf(stderr, 
            "handleLsda(...):lsda is non-zero.\n");
#endif

    // Get the current instruction pointer and offset it before next
    // instruction in the current frame which threw the exception.
    uintptr_t pc = _Unwind_GetIP(context)-1;

    // Get beginning current frame's code (as defined by the 
    // emitted dwarf code)
    uintptr_t funcStart = _Unwind_GetRegionStart(context);
    uintptr_t pcOffset = pc - funcStart;
    void** classInfo = NULL;

    // Note: See JITDwarfEmitter::EmitExceptionTable(...) for corresponding
    //       dwarf emission

    // Parse LSDA header.
    uint8_t lpStartEncoding = *lsda++;

    if (lpStartEncoding != DW_EH_PE_omit) {
        readEncodedPointer(&lsda, lpStartEncoding); 
    }

    uint8_t ttypeEncoding = *lsda++;
    uintptr_t classInfoOffset;

    if (ttypeEncoding != DW_EH_PE_omit) {
        // Calculate type info locations in emitted dwarf code which
        // were flagged by type info arguments to llvm.eh.selector
        // intrinsic
        classInfoOffset = readULEB128(&lsda);
        classInfo = (void **) (lsda + classInfoOffset);
    }

    // Walk call-site table looking for range that 
    // includes current PC. 

    uint8_t         callSiteEncoding = *lsda++;
    uint32_t        callSiteTableLength = readULEB128(&lsda);
    const uint8_t*  callSiteTableStart = lsda;
    const uint8_t*  callSiteTableEnd = callSiteTableStart + 
                                                    callSiteTableLength;
    const uint8_t*  actionTableStart = callSiteTableEnd;
    const uint8_t*  callSitePtr = callSiteTableStart;

    bool foreignException = false;

    while (callSitePtr < callSiteTableEnd) {
        uintptr_t start = readEncodedPointer(&callSitePtr, 
                                             callSiteEncoding);
        uintptr_t length = readEncodedPointer(&callSitePtr, 
                                              callSiteEncoding);
        uintptr_t landingPad = readEncodedPointer(&callSitePtr, 
                                                  callSiteEncoding);

        // Note: Action value
        uintptr_t actionEntry = readULEB128(&callSitePtr);

        if (exceptionClass != crack::runtime::crackClassId) {
            // We have been notified of a foreign exception being thrown,
            // and we therefore need to execute cleanup landing pads
            actionEntry = 0;
            foreignException = true;
        }

        if (landingPad == 0) {
#ifdef DEBUG
            fprintf(stderr,
                    "handleLsda(...): No landing pad found.\n");
#endif

            continue; // no landing pad for this entry
        }

        if (actionEntry) {
            actionEntry += ((uintptr_t) actionTableStart) - 1;
        }
        else {
#ifdef DEBUG
            fprintf(stderr,
                    "handleLsda(...):No action table found.\n");
#endif
        }

        bool exceptionMatched = false;

        if ((start <= pcOffset) && (pcOffset < (start + length))) {
#ifdef DEBUG
            fprintf(stderr,
                    "handleLsda(...): Landing pad found.\n");
#endif
            int64_t actionValue = 0;

            if (actionEntry) {
                exceptionMatched = handleActionValue
                                   (
                                       &actionValue,
                                       ttypeEncoding,
                                       classInfo, 
                                       actionEntry, 
                                       exceptionClass, 
                                       exceptionObject
                                   );
            }

            if (!(actions & _UA_SEARCH_PHASE)) {
#ifdef DEBUG
                fprintf(stderr,
                        "handleLsda(...): installed landing pad "
                            "context.\n");
#endif

                // Found landing pad for the PC.
                // Set Instruction Pointer to so we re-enter function 
                // at landing pad. The landing pad is created by the 
                // compiler to take two parameters in registers.
                _Unwind_SetGR(context, 
                              __builtin_eh_return_data_regno(0), 
                              (uintptr_t)exceptionObject);

                // Note: this virtual register directly corresponds
                //       to the return of the llvm.eh.selector intrinsic
                if (!actionEntry || !exceptionMatched) {
                    // We indicate cleanup only
                    _Unwind_SetGR(context, 
                                  __builtin_eh_return_data_regno(1), 
                                  0);
                }
                else {
                    // Matched type info index of llvm.eh.selector intrinsic
                    // passed here.
                    _Unwind_SetGR(context, 
                                  __builtin_eh_return_data_regno(1), 
                                  actionValue);
                }

                // To execute landing pad set here
                _Unwind_SetIP(context, funcStart + landingPad);
                ret = _URC_INSTALL_CONTEXT;
            }
            else if (exceptionMatched) {
#ifdef DEBUG
                fprintf(stderr,
                        "handleLsda(...): setting handler found.\n");
#endif
                ret = _URC_HANDLER_FOUND;
            }
            else {
                // Note: Only non-clean up handlers are marked as
                //       found. Otherwise the clean up handlers will be 
                //       re-found and executed during the clean up 
                //       phase.
#ifdef DEBUG
                fprintf(stderr,
                        "handleLsda(...): cleanup handler found.\n");
#endif
            }

            break;
        }
    }

    return(ret);
}
    
}} // namespace crack::runtime
