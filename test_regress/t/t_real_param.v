module foo
#( parameter real param = 2.0)
();

endmodule

module t();

   genvar m, r;
   generate
      for (m = 10; m <= 20; m+=10) begin : gen_m
         for (r = 0; r <= 1; r++) begin : gen_r
            localparam real lparam = m + (r + 0.5);
            initial begin
                $display("%m lparam = %f foo param = %f",
                         lparam, foo_inst.param);
                if (lparam != foo_inst.param) $stop();
            end

            foo #(.param (lparam)) foo_inst ();
         end
      end
   endgenerate

   initial begin
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule
