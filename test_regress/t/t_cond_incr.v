// DESCRIPTION: Verilator: Pre / post increment / decrement examples, both
// emitted and static
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Todd Strader.

module t (/*AUTOARG*/);

   function integer post_incr (
      input integer x,
      input integer y
   );
      integer result;
   
      result = 0;
   
      if (x++ > y)
         result = result + y;
   
      result = result + x;
   
      return result;
   endfunction
   
   function integer post_decr (
      input integer x,
      input integer y
   );
      integer result;
   
      result = 0;
   
      if (x-- < y)
         result = result + y;
   
      result = result + x;
   
      return result;
   endfunction
   
   function integer pre_incr (
      input integer x,
      input integer y
   );
      integer result;
   
      result = 0;
   
      if (++x > y)
         result = result + y;
   
      result = result + x;
   
      return result;
   endfunction
   
   function integer pre_decr (
      input integer x,
      input integer y
   );
      integer result;
   
      result = 0;
   
      if (--x < y)
         result = result + y;
   
      result = result + x;
   
      return result;
   endfunction

   localparam POST_INCR_RES = post_incr(5, 5);
   localparam POST_DECR_RES = post_decr(5, 5);
   localparam PRE_INCR_RES = pre_incr(5, 5);
   localparam PRE_DECR_RES = pre_decr(5, 5);

   integer post_incr_res;
   integer post_decr_res;
   integer pre_incr_res;
   integer pre_decr_res;

   integer five /*verilator public*/; // Ensure functional code is emitted

   initial begin
      five = 5;

      post_incr_res = post_incr(five, 5);
      post_decr_res = post_decr(five, 5);
      pre_incr_res = pre_incr(five, 5);
      pre_decr_res = pre_decr(five, 5);

      $display("post_incr result = %0d", post_incr_res);
      $display("post_decr result = %0d", post_decr_res);
      $display("pre_incr result = %0d", pre_incr_res);
      $display("pre_decr result = %0d", pre_decr_res);

      $display("post_incr static result = %0d", POST_INCR_RES);
      $display("post_decr static result = %0d", POST_DECR_RES);
      $display("pre_incr static result = %0d", PRE_INCR_RES);
      $display("pre_decr static result = %0d", PRE_DECR_RES);

      if (post_incr_res != 6) $stop;
      if (post_decr_res != 4) $stop;
      if (pre_incr_res != 11) $stop;
      if (pre_decr_res != 9) $stop;

      if (POST_INCR_RES != 6) $stop;
      if (POST_DECR_RES != 4) $stop;
      if (PRE_INCR_RES != 11) $stop;
      if (PRE_DECR_RES != 9) $stop;
    
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

