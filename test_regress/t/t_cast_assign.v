// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2017 by Todd Strader.

`define CAST(x, y) \
   $bits(x)'(y)

`define ASSIGN_FOO(x, y) \
   assign x.foo = y;

interface foo_intf;
    logic [7:0] foo;
endinterface

module t2 (
   input [31:0] i_data
);

   logic [7:0] foo;
   foo_intf the_foo_intf ();

   //assign the_foo_intf.foo = $bits(foo)'(32'd5);
   `ASSIGN_FOO(the_foo_intf, `CAST(foo, i_data))

   initial begin
      if (the_foo_intf.foo != 5) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module t();

   t2
   t2
   (
      .i_data (5)
   );

endmodule
