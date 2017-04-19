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

typedef struct packed {
   logic [31:0] bar;
} some_struct_t;

interface bar_intf;
   some_struct_t the_struct;

   modport sink
   (
      input the_struct
   );
endinterface

module t2 (
   bar_intf.sink i_bar
);

   logic [7:0] foo;
   foo_intf the_foo_intf ();

   //assign the_foo_intf.foo = $bits(foo)'(32'd5);
   `ASSIGN_FOO(the_foo_intf, `CAST(foo, i_bar.the_struct.bar))

   initial begin
      if (the_foo_intf.foo != 5) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module t();

   bar_intf the_bar_intf ();

   assign the_bar_intf.the_struct.bar = 32'd5;

   t2
   t2
   (
      .i_bar (the_bar_intf)
   );

endmodule
