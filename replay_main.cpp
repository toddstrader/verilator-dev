#include "verilated_replay.h"
#include "Vt_case_reducer.h"
#include "verilated_vcd_c.h"

double simTime = 0;
double sc_time_stamp() {
    return simTime;
}

Vt_case_reducer* dutp = NULL;
VerilatedVcdC* tfp = NULL;
// TODO -- should we make eval, final and trace(?) part of VerilatedModule?
void VerilatedReplay::eval() {
    dutp->eval();
}

void VerilatedReplay::trace() {
#if VM_TRACE
    if (tfp) tfp->dump(simTime);
#endif  // VM_TRACE
}

void VerilatedReplay::final() {
    dutp->final();
}

int main(int argc, char** argv) {
    std::string vcdFilename(argv[1]);
    std::string scopeName(argv[2]);
    VL_PRINTF("VCD = %s scope = %s\n", vcdFilename.c_str(), scopeName.c_str());

    VerilatedReplay replay(vcdFilename, scopeName);

    dutp = new Vt_case_reducer;
    if (replay.addInput("clk", &dutp->clk, 1)) exit(-1);
    // TODO -- can we get rid of the "[:]"?
    if (replay.addInput("operand_a[:]", &dutp->operand_a, 8)) exit(-1);
    if (replay.addInput("operand_b[:]", &dutp->operand_b, 8)) exit(-1);

#if VM_TRACE
    Verilated::traceEverOn(true);
    tfp = new VerilatedVcdC;
    dutp->trace(tfp, 99);
    tfp->open("replay.vcd");
    if (tfp) tfp->dump(simTime);
#endif  // VM_TRACE

    if (replay.replay(simTime)) exit(-1);

#if VM_TRACE
    if (tfp) tfp->close();
#endif  // VM_TRACE

    return 0;
}
