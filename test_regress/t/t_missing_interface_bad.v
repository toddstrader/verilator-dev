// DESCRIPTION: Verilator: Missing interface test
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Todd Strader.

// Interface intentionally not defined
//interface foo_intf;
//   logic a;
//endinterface

//`define BASELINE

`ifndef BASELINE
module foo_mod
  (
   foo_intf foo
   );
endmodule
`endif

module t (/*AUTOARG*/);

   foo_intf the_foo ();

`ifndef BASELINE
   foo_mod
     foo_mod
       (
	.foo (the_foo)
	);
`endif

   initial begin
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

