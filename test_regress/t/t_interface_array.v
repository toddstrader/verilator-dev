// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Todd Strader.

interface foo_intf;
    logic a;

    modport source (
        output a
    );

    modport sink (
        input a
    );
endinterface

module t (/*AUTOARG*/
   // Inputs
   clk
   );

   input clk;

    localparam N = 4;

    logic [N-1:0] a_in;
    logic [N-1:0] a_out;
    logic [N-1:0] ack_out;

    foo_intf foos [N-1:0] ();

    generate
    genvar i;
        for (i = 0; i < N; i++) begin : someLoop
            assign ack_out[i] = a_in[i];
            assign foos[i].a = a_in[i];
            assign a_out[i] = foos[i].a;
        end
    endgenerate

    always @(posedge clk) begin
        assert(ack_out == a_out) else
            $fatal(2, "Interface and non-interface paths do not match: 0b%b 0b%b",
                ack_out, a_out);
    end

    initial a_in = '0;
    always @(posedge clk) begin
        a_in <= a_in + { {N-1 {1'b0}}, 1'b1 };

        if (& a_in) begin
            $write("*-* All Finished *-*\n");
            $finish;
        end
    end

endmodule

