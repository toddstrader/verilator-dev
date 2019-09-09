// DESCRIPTION: Verilator: Verilog Test module
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2019 by Todd Strader.

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

    genvar x;
    generate
        for (x = 0; x < 2; x = x + 1) begin: gen_loop
            integer cyc = 0;
            logic [31:0] accum_in;
            logic [31:0] accum_out;
            logic s1_in;
            logic s1_out;
            logic [1:0] s2_in;
            logic [1:0] s2_out;
            logic [7:0] s8_in;
            logic [7:0] s8_out;
            logic [32:0] s33_in;
            logic [32:0] s33_out;
            logic [63:0] s64_in;
            logic [63:0] s64_out;
            logic [64:0] s65_in;
            logic [64:0] s65_out;
            logic [128:0] s129_in;
            logic [128:0] s129_out;

            dpi_prot_secret
            secret (
                .accum_in,
                .accum_out,
                .s1_in,
                .s1_out,
                .s2_in,
                .s2_out,
                .s8_in,
                .s8_out,
                .s33_in,
                .s33_out,
                .s64_in,
                .s64_out,
                .s65_in,
                .s65_out,
                .s129_in,
                .s129_out,
                .clk);

            always @(posedge clk) begin
`ifdef TEST_VERBOSE
                $write("[%0t] x=%0d, cyc=%0d accum_in=%0d accum_out=%0d\n",
                       $time, x, cyc, accum_in, accum_out);
`endif
                cyc <= cyc + 1;
                accum_in <= accum_in + 5;
                if (cyc == 0) begin
                    accum_in <= x*100;
                end else if (cyc == 10) begin
                    $write("*-* All Finished *-*\n");
                    $finish;
                end
            end
        end
    endgenerate

endmodule
