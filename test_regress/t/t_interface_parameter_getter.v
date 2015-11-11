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
		      .i (i.mp),
                      .i_no_mp (i)
                      );

endmodule


module testmod
  (
   input clk,
   test_if.mp  i,
   test_if i_no_mp
   );

   localparam THE_FOO = i.getFoo();
   localparam THE_OTHER_FOO = i_no_mp.getFoo();

   always @(posedge clk) begin
      if (THE_FOO != 5) begin
         $display("%%Error: THE_FOO = %0d", THE_FOO);
	 $stop;
      end
      if (THE_OTHER_FOO != 5) begin
         $display("%%Error: THE_OTHER_FOO = %0d", THE_OTHER_FOO);
	 $stop;
      end
      if (i.getFoo() != 5) begin
         $display("%%Error: i.getFoo() = %0d", i.getFoo());
	 $stop;
      end
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
