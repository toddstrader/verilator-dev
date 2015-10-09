// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Todd Strader.

interface foo_intf;
    logic a;
endinterface

function integer identity (input integer val);
    return val;
endfunction

module t (/*AUTOARG*/);

    foo_intf foos [0:0] ();

//    generate
//        genvar i;
//        for (i = 0; i < 1; i++) begin
            assign foos[0].a = 1'b1;
//            assign foos[identity(0)].a = 1'b1;
//        end
//    endgenerate

    initial begin
        $write("*-* All Finished *-*\n");
        $finish;
    end

endmodule

