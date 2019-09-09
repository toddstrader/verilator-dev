// DESCRIPTION: Verilator: Verilog Test module
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2019 by Todd Strader.

module secret_impl (
    input [31:0] accum_in,
    output [31:0] accum_out,
    input s1_in,
    output logic s1_out,
    input [1:0] s2_in,
    output logic [1:0] s2_out,
    input [7:0] s8_in,
    output logic [7:0] s8_out,
    input [32:0] s33_in,
    output logic [32:0] s33_out,
    input [63:0] s64_in,
    output logic [63:0] s64_out,
    input [64:0] s65_in,
    output logic [64:0] s65_out,
    input [128:0] s129_in,
    output logic [128:0] s129_out,
    // TODO -- structs, packed, unpacked arrays
    input clk);

    logic [31:0] accum_q = '0;

    always @(accum_in) $write("%m accum_in=%0d\n", accum_in);
    always @(clk) $write("%m clk=%0d\n", clk);
    always @(posedge clk) begin
        $write("[%0t] (%m) accum_in=%0d accum_out=%0d\n",
               $time, accum_in, accum_out);
        accum_q <= accum_q + accum_in;
        s1_out <= s1_in;
        s2_out <= s2_in;
        s8_out <= s8_in;
        s33_out <= s33_in;
        s64_out <= s64_in;
        s65_out <= s65_in;
        s129_out <= s129_in;
    end

    assign accum_out = accum_q;

endmodule
