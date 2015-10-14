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
    genvar the_genvar;
        for (the_genvar = 0; the_genvar < 1; the_genvar++) begin : someLoop
            assign foos[the_genvar].a = a_in[the_genvar];
        end
    endgenerate

    initial begin
        $write("*-* All Finished *-*\n");
        $finish;
    end

endmodule

