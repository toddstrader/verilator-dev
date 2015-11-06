// DESCRIPTION: Verilator: Interface parameter getter
//
// A test of the import parameter used with modport
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Todd Strader

interface test_if #(parameter integer FOO = 1);

   // Interface variable
   logic 	data;

   // Modport
   modport mp(
              import  getFoo,
	      output  data
	      );

   function integer getFoo ();
       return FOO;
   endfunction

endinterface // test_if


module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

   test_if #( .FOO (5) ) i ();

   testmod testmod_i (.clk (clk),
		      .i (i.mp));

endmodule


module testmod
  (
   input clk,
   test_if.mp  i
   );

   localparam THE_FOO = i.getFoo();

   always @(posedge clk) begin
      if (THE_FOO == 5) begin
	 $write("*-* All Finished *-*\n");
	 $finish;
      end
      else begin
         $display("%%Error: THE_FOO = %0d", THE_FOO);
	 $stop;
      end
   end
endmodule
