// @HEADER
// ************************************************************************
//
//               Digital Image Correlation Engine (DICe)
//                 Copyright 2015 Sandia Corporation.
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact: Dan Turner (dzturne@sandia.gov)
//
// ************************************************************************
// @HEADER

#include <DICe.h>
#include <DICe_Image.h>
#include <DICe_Shape.h>

#include <Teuchos_RCP.hpp>
#include <Teuchos_oblackholestream.hpp>
#include <Teuchos_ParameterList.hpp>

#include <iostream>

using namespace DICe;

int main(int argc, char *argv[]) {

  DICe::initialize(argc, argv);

  // only print output if args are given (for testing the output is quiet)
  int_t iprint     = argc - 1;
  int_t errorFlag  = 0;
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  *outStream << "--- Begin test ---" << std::endl;

  // create an image from file:
  *outStream << "creating an image from a tiff file " << std::endl;
  Teuchos::RCP<Image> img = Teuchos::rcp(new Image("./images/ImageA.tif"));
  img->write_tiff("outImageA.tif");
  if(img->width()!=2048){
    *outStream << "Error, the image width is not correct" << std::endl;
    errorFlag +=1;
  }
  if(img->height()!=589){
    *outStream << "Error, the image height is not correct" << std::endl;
    errorFlag +=1;
  }

  // test the mask values for an image:
  *outStream << "testing image mask" << std::endl;
  // create a mask using a simple rectangle
  Teuchos::RCP<DICe::Rectangle> rect1 = Teuchos::rcp(new DICe::Rectangle(1024,294,400,200));
  Teuchos::RCP<DICe::Rectangle> rect2 = Teuchos::rcp(new DICe::Rectangle(1024,294,100,50));
  DICe::multi_shape boundary;
  boundary.push_back(rect1);
  DICe::multi_shape excluded;
  excluded.push_back(rect2);
  DICe::Conformal_Area_Def area_def(boundary,excluded);
  img->create_mask(area_def,true);

  // create an image of the mask
  Teuchos::ArrayRCP<intensity_t> mask_values(img->height()*img->width(),0.0);
  for(int_t y=0;y<img->height();++y)
    for(int_t x=0;x<img->width();++x)
      mask_values[y*img->width()+x] = img->mask(x,y);
  Image mask(img->width(),img->height(),mask_values);
  // compare with the saved mask file
  Teuchos::RCP<Image> mask_exact = Teuchos::rcp(new Image("./images/mask.rawi"));
  const scalar_t mask_diff = mask.diff(mask_exact);
  *outStream << "mask value diff: " << mask_diff << std::endl;
  const scalar_t mask_tol = 1.0E-2;
  if(mask_diff > mask_tol){
    *outStream << "Error, mask values not correct" << std::endl;
    errorFlag++;
  }
  *outStream << "image mask values have been checked" << std::endl;

  // capture a portion of an image from file
  *outStream << "creating an image from a portion of a tiff file " << std::endl;
  Image sub_img("./images/ImageA.tif",100,100,300,200);
  sub_img.write_tiff("outSubImageA.tif");
  if(sub_img.width()!=300){
    *outStream << "Error, the sub image width is not correct" << std::endl;
    errorFlag +=1;
  }
  if(sub_img.height()!=200){
    *outStream << "Error, the sub image height is not correct" << std::endl;
    errorFlag +=1;
  }
  // the pixel values from the sub image should line up with the image above, if given the right coordinates
  bool intensity_match_error = false;
  for(int_t y=0;y<sub_img.height();++y){
    for(int_t x=0;x<sub_img.width();++x){
      if(sub_img(x,y)!=(*img)(x+sub_img.offset_x(),y+sub_img.offset_y()))
        intensity_match_error = true;
    }
  }
  if(intensity_match_error){
    *outStream << "Error, the intensities for the sub image do not match the global image" << std::endl;
    errorFlag+=1;
  }

  *outStream << "creating a sub-image" << std::endl;
  // purposefully making the image extend beyond the bounds of the input image
  Teuchos::RCP<Image> portion = Teuchos::rcp(new Image(img,img->width()/2,img->height()/2,img->width(),img->height()));
  //portion->write_rawi("portion.rawi");
  Teuchos::RCP<Image> portion_exact = Teuchos::rcp(new Image("./images/portion.rawi"));
  scalar_t portion_diff = portion_exact->diff(portion);
  if(portion_diff > mask_tol){
    *outStream << "Error, the portion image is not correct" << std::endl;
    errorFlag++;
  }

  // create an image from an array
  *outStream << "creating an image from an array" << std::endl;
  const int_t array_w = 30;
  const int_t array_h = 20;
  intensity_t * intensities = new intensity_t[array_w*array_h];
  scalar_t * gx = new scalar_t[array_w*array_h];
  scalar_t * gy = new scalar_t[array_w*array_h];
  // populate the intensities with a sin/cos function
  for(int_t y=0;y<array_h;++y){
    for(int_t x=0;x<array_w;++x){
      intensities[y*array_w+x] = 255*std::cos(x/(4*DICE_PI))*std::sin(y/(4*DICE_PI));
      gx[y*array_w+x] = -255*(1/(4*DICE_PI))*std::sin(x/(4*DICE_PI))*std::sin(y/(4*DICE_PI));
      gy[y*array_w+x] = 255*(1/(4*DICE_PI))*std::cos(x/(4*DICE_PI))*std::cos(y/(4*DICE_PI));
    }
  }
  Teuchos::RCP<Teuchos::ParameterList> params = rcp(new Teuchos::ParameterList());
  params->set(DICe::compute_image_gradients,true);
  Image array_img(intensities,array_w,array_h,params);

  *outStream << "creating an image from an array RCP" << std::endl;
  Teuchos::ArrayRCP<intensity_t> intensityRCP(intensities,0,array_w*array_h,false);
  Image rcp_img(array_w,array_h,intensityRCP);

  bool intensity_value_error = false;
  bool grad_x_error = false;
  bool grad_y_error = false;
  scalar_t grad_tol = 1.0E-3;
  for(int_t y=2;y<array_h-2;++y){
    for(int_t x=2;x<array_w-2;++x){
      if(intensities[y*array_w+x] != array_img(x,y))
        intensity_value_error = true;
      //std::cout << " grad x " << gx[y*array_w+x] << " computed " << array_img.grad_x(x,y) << std::endl;
      if(std::abs(gx[y*array_w+x] - array_img.grad_x(x,y)) > grad_tol)
        grad_x_error = true;
      if(std::abs(gy[y*array_w+x] - array_img.grad_y(x,y)) > grad_tol)
        grad_y_error = true;
    }
  }
  if(intensity_value_error){
    *outStream << "Error, the intensity values are wrong" << std::endl;
    errorFlag++;
  }
  if(grad_x_error){
    *outStream << "Error, the flat x-gradient values are wrong" << std::endl;
    errorFlag++;
  }
  if(grad_y_error){
    *outStream << "Error, the flat y-gradient values are wrong" << std::endl;
    errorFlag++;
  }
  *outStream << "flat image gradients have been checked" << std::endl;

  grad_x_error = false;
  grad_y_error = false;
  // check the hierarchical gradients:
  const int_t team_size = 256;
  array_img.compute_gradients(true,team_size);
  for(int_t y=2;y<array_h-2;++y){
    for(int_t x=2;x<array_w-2;++x){
      if(std::abs(gx[y*array_w+x] - array_img.grad_x(x,y)) > grad_tol)
        grad_x_error = true;
      if(std::abs(gy[y*array_w+x] - array_img.grad_y(x,y)) > grad_tol)
        grad_y_error = true;
    }
  }
  if(grad_x_error){
    *outStream << "Error, hierarchical x-gradient values are wrong" << std::endl;
    errorFlag++;
  }
  if(grad_y_error){
    *outStream << "Error, the hierarchical y-gradient values are wrong" << std::endl;
    errorFlag++;
  }
  *outStream << "hierarchical image gradients have been checked" << std::endl;

  // create an image with a teuchos array and compare the values
  bool rcp_error = false;
  for(int_t y=0;y<array_h;++y){
    for(int_t x=0;x<array_w;++x){
      if(rcp_img(x,y) != array_img(x,y))
        rcp_error = true;
    }
  }
  if(rcp_error){
    *outStream << "Error, the rcp image was not created correctly" << std::endl;
    errorFlag++;
  }

  delete[] intensities;
  delete[] gx;
  delete[] gy;

  // test image transform:
  *outStream << "testing an image transform" << std::endl;
  Image baboon("./images/baboon.rawi");
  const int_t cx = 100;
  const int_t cy = 100;
  const scalar_t u = 25.0;
  const scalar_t v = -15.2;
  const scalar_t theta = 22.8*DICE_PI/180.0;
  Teuchos::RCP<std::vector<scalar_t> > def = Teuchos::rcp(new std::vector<scalar_t>(DICE_DEFORMATION_SIZE,0.0));
  (*def)[DISPLACEMENT_X] = u;
  (*def)[DISPLACEMENT_Y] = v;
  (*def)[ROTATION_Z] = theta;
  Teuchos::RCP<Image> trans_baboon = baboon.apply_transformation(def,cx,cy,false);
  //trans_baboon->write_rawi("baboon_trans.rawi");
  //trans_baboon->write_tiff("baboon_trans.tif");
  Teuchos::RCP<Image> trans_baboon_exact = Teuchos::rcp(new Image("./images/baboon_trans.rawi"));
  //trans_baboon_exact->write_tiff("baboon_trans_exact.tif");
  const scalar_t diff_trans_baboon = trans_baboon->diff(trans_baboon_exact);
  *outStream << "image diff trans baboon: " << diff_trans_baboon << std::endl;
  const scalar_t diff_tol = 0.1;
  if(diff_trans_baboon > diff_tol){
    *outStream << "Error, the transformed image does not have the right intensities" << std::endl;
    errorFlag++;
  }

  *outStream << "testing 180 rotation in place" << std::endl;
  Teuchos::RCP<Image> baboon_180 = baboon.apply_rotation(ONE_HUNDRED_EIGHTY_DEGREES);
  //baboon.write_rawi("baboon_180.rawi");
  Teuchos::RCP<Image> baboon_180_exact = Teuchos::rcp(new Image("./images/baboon_180.rawi"));
  const scalar_t diff_180 = baboon_180->diff(baboon_180_exact);
  if(diff_180 > diff_tol){
    *outStream << "Error, the 180 degree transformed image does not have the right intensities." << std::endl;
    errorFlag++;
  }
  *outStream << "the transformation method for an image has been checked" << std::endl;

  // test 90 and 270 degree rotation

  *outStream << "testing 90 and 270 degree rotations" << std::endl;
  Teuchos::RCP<Image> img_0_deg = Teuchos::rcp(new Image("./images/ImageB.tif"));
  Teuchos::RCP<Image> img_90_deg = img_0_deg->apply_rotation(NINTY_DEGREES);
  //img_90_deg->write_tiff("img_90.tif");
  //img_90_deg->write_rawi("img_90.rawi");
  Teuchos::RCP<Image> img_270_deg = img_0_deg->apply_rotation(TWO_HUNDRED_SEVENTY_DEGREES);
  //img_270_deg->write_tiff("img_270.tif");
  //img_270_deg->write_rawi("img_270.rawi");
  Teuchos::RCP<Image> img_90_exact = Teuchos::rcp(new Image("./images/img_90.rawi"));
  const scalar_t diff_90 = img_90_deg->diff(img_90_exact);
  if(diff_90 > 1.0E-4){
    *outStream << "Error, the 90 degree transformed image does not have the right intensities." << std::endl;
    errorFlag++;
  }
  Teuchos::RCP<Image> img_270_exact = Teuchos::rcp(new Image("./images/img_270.rawi"));
  const scalar_t diff_270 = img_270_deg->diff(img_270_exact);
  if(diff_270 > 1.0E-4){
    *outStream << "Error, the 270 degree transformed image does not have the right intensities." << std::endl;
    errorFlag++;
  }
  *outStream << "rotations have been tested" << std::endl;

  // test filtering an image:

  *outStream << "creating gauss filtered image filter size 5: outFilter5.tif " << std::endl;
  Teuchos::RCP<Teuchos::ParameterList> filter_5_params = rcp(new Teuchos::ParameterList());
  filter_5_params->set(DICe::gauss_filter_images,true);
  filter_5_params->set(DICe::gauss_filter_mask_size,5);
  Teuchos::RCP<Image> filter_5_img = Teuchos::rcp(new Image("./images/ImageB.tif",filter_5_params));
  //filter_5_img.write_tiff("outFilter5.tif");
  //filter_5_img.write_rawi("outFilter5.rawi");
  Teuchos::RCP<Image> filter_5_exact = Teuchos::rcp(new Image("./images/outFilter5.rawi"));
  const scalar_t diff_5 = filter_5_exact->diff(filter_5_img);
  if(diff_5 > diff_tol){
    *outStream << "Error, the 5 pixel filter image does not have the right intensities." << std::endl;
    errorFlag++;
  }

  *outStream << "creating gauss filtered image filter size 7: outFilter7.tif " << std::endl;
  Teuchos::RCP<Teuchos::ParameterList> filter_7_params = rcp(new Teuchos::ParameterList());
  filter_7_params->set(DICe::gauss_filter_images,true);
  filter_7_params->set(DICe::gauss_filter_mask_size,7);
  Teuchos::RCP<Image> filter_7_img = Teuchos::rcp(new Image("./images/ImageB.tif",filter_7_params));
  //filter_7_img.write_tiff("outFilter7.tif");
  //filter_7_img.write_rawi("outFilter7.rawi");
  Teuchos::RCP<Image> filter_7_exact = Teuchos::rcp(new Image("./images/outFilter7.rawi"));
  const scalar_t diff_7 = filter_7_exact->diff(filter_7_img);
  if(diff_7 > diff_tol){
    *outStream << "Error, the 7 pixel filter image does not have the right intensities." << std::endl;
    errorFlag++;
  }

  *outStream << "creating gauss filtered image filter size 9: outFilter9.tif " << std::endl;
  Teuchos::RCP<Teuchos::ParameterList> filter_9_params = rcp(new Teuchos::ParameterList());
  filter_9_params->set(DICe::gauss_filter_images,true);
  filter_9_params->set(DICe::gauss_filter_mask_size,9);
  Teuchos::RCP<Image> filter_9_img = Teuchos::rcp(new Image("./images/ImageB.tif",filter_9_params));
  //filter_9_img.write_tiff("outFilter9.tif");
  //filter_9_img.write_rawi("outFilter9.rawi");
  Teuchos::RCP<Image> filter_9_exact = Teuchos::rcp(new Image("./images/outFilter9.rawi"));
  const scalar_t diff_9 = filter_9_exact->diff(filter_9_img);
  if(diff_9 > diff_tol){
    *outStream << "Error, the 9 pixel filter image does not have the right intensities." << std::endl;
    errorFlag++;
  }

  *outStream << "creating gauss filtered image filter size 11: outFilter11.tif " << std::endl;
  Teuchos::RCP<Teuchos::ParameterList> filter_11_params = rcp(new Teuchos::ParameterList());
  filter_11_params->set(DICe::gauss_filter_images,true);
  filter_11_params->set(DICe::gauss_filter_mask_size,11);
  Teuchos::RCP<Image> filter_11_img = Teuchos::rcp(new Image("./images/ImageB.tif",filter_11_params));
  //filter_11_img.write_tiff("outFilter11.tif");
  //filter_11_img.write_rawi("outFilter11.rawi");
  Teuchos::RCP<Image> filter_11_exact = Teuchos::rcp(new Image("./images/outFilter11.rawi"));
  const scalar_t diff_11 = filter_11_exact->diff(filter_11_img);
  if(diff_11 > diff_tol){
    *outStream << "Error, the 11 pixel filter image does not have the right intensities." << std::endl;
    errorFlag++;
  }

  *outStream << "creating gauss filtered image filter size 13: outFilter13.tif " << std::endl;
  Teuchos::RCP<Teuchos::ParameterList> filter_13_params = rcp(new Teuchos::ParameterList());
  filter_13_params->set(DICe::gauss_filter_images,true);
  filter_13_params->set(DICe::gauss_filter_mask_size,13);
  Teuchos::RCP<Image> filter_13_img = Teuchos::rcp(new Image("./images/ImageB.tif",filter_13_params));
  //filter_13_img.write_tiff("outFilter13.tif");
  //filter_13_img.write_rawi("outFilter13.rawi");
  Teuchos::RCP<Image> filter_13_exact = Teuchos::rcp(new Image("./images/outFilter13.rawi"));
  const scalar_t diff_13 = filter_13_exact->diff(filter_13_img);
  if(diff_13 > diff_tol){
    *outStream << "Error, the 13 pixel filter image does not have the right intensities." << std::endl;
    errorFlag++;
  }

  *outStream << "creating gauss filtered image filter size 13 (with hierarchical parallelism) to compare to flat filter functor" << std::endl;
  Teuchos::RCP<Teuchos::ParameterList> filter_h_params = rcp(new Teuchos::ParameterList());
  filter_h_params->set(DICe::gauss_filter_images,true);
  filter_h_params->set(DICe::gauss_filter_use_hierarchical_parallelism,true);
  filter_h_params->set(DICe::gauss_filter_team_size,256);
  filter_h_params->set(DICe::gauss_filter_mask_size,13);
  Image filter_h_img("./images/ImageB.tif",filter_h_params);

  bool filter_error = false;
  for(int_t y=0;y<filter_h_img.height();++y){
    for(int_t x=0;x<filter_h_img.width();++x){
      if(std::abs(filter_h_img(x,y) - (*filter_13_img)(x,y)) > grad_tol)
        filter_error = true;
    }
  }
  if(filter_error){
    *outStream << "Error, the filtered image using the flat functor does not match the hierarchical one" << std::endl;
    errorFlag++;
  }
  *outStream << "hierarchical image filter has been checked" << std::endl;

  // test that unsupported file formats throw an exception

  bool exception_thrown = false;
  try{
    DICe::Image jpg("./images/invalid.jpg");
  }
  catch (const std::exception &e){
    exception_thrown = true;
    *outStream << ".jpg throws an exception as expected" << std::endl;
  }
  if(!exception_thrown){
    *outStream << "Error, an exception should have been thrown for invalid .jpg file format, but was not" << std::endl;
    errorFlag++;
  }
  exception_thrown = false;
  try{
    DICe::Image png("./images/invalid.png");
  }
  catch (const std::exception &e){
    exception_thrown = true;
    *outStream << ".png throws an exception as expected" << std::endl;
  }
  if(!exception_thrown){
    *outStream << "Error, an exception should have been thrown for invalid .png file format, but was not" << std::endl;
    errorFlag++;
  }
  exception_thrown = false;
  try{
    DICe::Image bmp("./images/invalid.bmp");
  }
  catch (const std::exception &e){
    exception_thrown = true;
    *outStream << ".bmp throws an exception as expected" << std::endl;
  }
  if(!exception_thrown){
    *outStream << "Error, an exception should have been thrown for invalid .bmp file format, but was not" << std::endl;
    errorFlag++;
  }

  *outStream << "--- End test ---" << std::endl;

  DICe::finalize();

  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  return 0;

}
