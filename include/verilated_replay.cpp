// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Copyright 2003-2019 by Todd Strader. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License.
// Version 2.0.
//
// Verilator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//=========================================================================
///
/// \file
/// \brief Verilator: Common functions for replay tool.
///
///     This utility will replay trace files onto a verilated design.
///
/// Code available from: http://www.veripool.org/verilator
///
//=========================================================================

#include "verilated_replay.h"
#include "verilated.h"

#include <sstream>

VerilatedReplay::VerilatedReplay(const std::string& vcdFilename, const std::string& scopeName) {

    VCDFileParser parser;
    m_vcdp = parser.parse_file(vcdFilename);
    if (!m_vcdp) {
        VL_PRINTF("Error parsing VCD\n");
        exit(-1);
    }

    std::vector<VCDScope*>* scopesp = m_vcdp->get_scopes();
    if (!scopesp) {
        VL_PRINTF("Error getting scopes\n");
        exit(-1);
    }
    VL_PRINTF("Scopes:\n");
    for (std::vector<VCDScope*>::iterator it = scopesp->begin();
         it != scopesp->end(); ++it) {
        VL_PRINTF("%s\n", (*it)->name.c_str());
    }

    // TODO -- would like to put this kind of scope finding in the
    //          VCD parsing library
    m_scopep = m_vcdp->root_scope;
    if (!m_scopep) {
        VL_PRINTF("Error getting root scope\n");
        exit(-1);
    }
    std::stringstream scopeStream(scopeName);
    std::string scopeToken;
    VL_PRINTF("Finding scope:\n");
    while (getline(scopeStream, scopeToken, '.')) {
        VL_PRINTF("%s\n", scopeToken.c_str());
        std::vector<VCDScope*>* childrenp = &m_scopep->children;
        m_scopep = NULL;
        for (std::vector<VCDScope*>::iterator it = childrenp->begin();
             it != childrenp->end(); ++it) {
            if ((*it)->name == scopeToken) {
                m_scopep = *it;
                break;
            }
        }
        if (!m_scopep) {
            VL_PRINTF("Error getting scope\n");
            exit(-1);
        }
    }

    VL_PRINTF("Signals:\n");
    for (std::vector<VCDSignal*>::iterator it = m_scopep->signals.begin();
         it != m_scopep->signals.end(); ++it) {
        VL_PRINTF("%s\n", (*it)->reference.c_str());
    }
}

VerilatedReplay::~VerilatedReplay() {
    for (VerilatedReplaySignals::iterator it = m_inputs.begin();
         it != m_inputs.end(); ++it) {
        delete *it;
    }
}

int VerilatedReplay::addInput(const std::string& signalName, void* dutSignal, unsigned bits) {
    for (std::vector<VCDSignal*>::iterator it = m_scopep->signals.begin();
         it != m_scopep->signals.end(); ++it) {
        if (signalName == (*it)->reference) {
            VerilatedReplaySignal* signalp = new VerilatedReplaySignal;
            signalp->dutSignal = dutSignal;
            signalp->bits = bits;
            signalp->hash = (*it)->hash;

            if ((*it)->size != bits) {
                VL_PRINTF("Error size mismatch on %s: trace=%d design=%d\n",
                          signalName.c_str(), (*it)->size, bits);
                return -1;
            }

            if ((*it)->type != VCD_VAR_REG && (*it)->type != VCD_VAR_WIRE) {
                VL_PRINTF("Error unsupported signal type on %s\n", signalName.c_str());
                return -1;
            }

            m_inputs.push_back(signalp);
            return 0;
        }
    }
    VL_PRINTF("Error finding signal (%s)\n", signalName.c_str());
    return -1;
}

void VerilatedReplay::copySignal(uint8_t* signal, VCDValue* valuep) {
    VCDValueType type = valuep->get_type();
    switch(type) {
        case VCD_SCALAR: {
            copySignalBit(signal, 0, valuep->get_value_bit());
            break;
        }
        case VCD_VECTOR: {
            VCDBitVector* bitVector = valuep->get_value_vector();
            unsigned length = bitVector->size();
            for (int i = 0; i < length; ++i) {
                copySignalBit(signal, i, bitVector->at(length - i - 1));
            }
            break;
        }
        default: {
            VL_PRINTF("Error unsupported VCD value type");
            exit(-1);
        }
    }
}

void VerilatedReplay::copySignalBit(uint8_t* signal, unsigned offset, VCDBit bit) {
    unsigned byte = offset / 8;
    unsigned byteOffset = offset % 8;
    // TODO - more efficient byte copying
    signal[byte] &= ~(0x1 << byteOffset);
    // TODO - x's and z's?
    signal[byte] |= (bit == VCD_1 ? 0x1 : 0x0) << byteOffset;
}

int VerilatedReplay::replay(double& simTime) {
    std::vector<VCDTime>* timesp = m_vcdp->get_timestamps();
    if (!timesp) {
        VL_PRINTF("Error getting VCD times\n");
        return -1;
    }

    for (std::vector<VCDTime>::iterator it = timesp->begin();
         it != timesp->end(); ++it) {
        simTime = *it;
        VL_PRINTF("Time = %f\n", simTime);
        for (VerilatedReplaySignals::iterator sig = m_inputs.begin();
             sig != m_inputs.end(); ++sig) {
            VCDValue* valuep = m_vcdp->get_signal_value_at((*sig)->hash, simTime);
            copySignal(reinterpret_cast<uint8_t*>((*sig)->dutSignal), valuep);
        }

        eval();
        trace();
    }

    final();

    return 0;
}
