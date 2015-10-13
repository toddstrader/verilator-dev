// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Todd Strader.

interface foo_intf;
    logic a;
endinterface

module t (/*AUTOARG*/);

    logic [0:0] a_in;

    foo_intf foos [0:0] ();

    generate
    genvar i;
        for (i = 0; i < 1; i++) begin : someLoop
            assign foos[i].a = a_in[i];
        end
    endgenerate

    initial begin
        $write("*-* All Finished *-*\n");
        $finish;
    end

endmodule

