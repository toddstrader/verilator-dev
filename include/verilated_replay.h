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
/// \brief Verilator: Include for replay tool
///
///     This utility will replay trace files onto a verilated design.
///
/// Code available from: http://www.veripool.org/verilator
///
//=========================================================================


#ifndef _VERILATED_REPLAY_H_
#define _VERILATED_REPLAY_H_ 1  ///< Header Guard

#include "VCDFileParser.hpp"

class VerilatedReplaySignal {
public:
    void* dutSignal;
    unsigned bits;
    VCDSignalHash hash;
};

typedef std::vector<VerilatedReplaySignal*> VerilatedReplaySignals;

class VerilatedReplay {
private:
    VCDFile* m_vcdp;
    VCDScope* m_scopep;
    VerilatedReplaySignals m_inputs;
    void eval();
    void trace();
    void final();
    void copySignal(uint8_t* signal, VCDValue* valuep);
    void copySignalBit(uint8_t* signal, unsigned offset, VCDBit bit);
public:
    VerilatedReplay(const std::string& vcdFilename, const std::string& scopeName);
    ~VerilatedReplay();
    int addInput(const std::string& signalName, void* dutSignal, unsigned bits);
    int replay(double& simTime);
};

#endif  // Guard
