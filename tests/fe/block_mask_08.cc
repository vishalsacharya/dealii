// ---------------------------------------------------------------------
//
// Copyright (C) 2012 - 2017 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------



// tests for the BlockMask class
//
// here: BlockMask::represents_the_all_selected_mask


#include "../tests.h"
#include <deal.II/fe/block_mask.h>





void test ()
{
  // test for an initialized mask
  AssertThrow (BlockMask(12,false).represents_the_all_selected_mask() == false,
               ExcInternalError());
  // note the semantics of the following as
  // described in the documentation
  AssertThrow (BlockMask(12,true).represents_the_all_selected_mask() == false,
               ExcInternalError());
  // test for an empty mask
  AssertThrow (BlockMask().represents_the_all_selected_mask() == true,
               ExcInternalError());

  deallog << "OK" << std::endl;
}


int main()
{
  std::ofstream logfile ("output");
  deallog << std::setprecision (4);

  deallog.attach(logfile);

  test();
}
