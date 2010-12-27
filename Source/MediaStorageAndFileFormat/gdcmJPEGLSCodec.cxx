/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2010 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmJPEGLSCodec.h"
#include "gdcmTransferSyntax.h"
#include "gdcmSequenceOfFragments.h"
#include "gdcmDataElement.h"

// CharLS includes
#include "gdcmcharls/interface.h"
#include "gdcmcharls/util.h"
#include "gdcmcharls/defaulttraits.h"
#include "gdcmcharls/losslesstraits.h"
#include "gdcmcharls/colortransform.h"
#include "gdcmcharls/streams.h"
#include "gdcmcharls/processline.h"
#include "gdcmcharls/publictypes.h"


namespace gdcm
{

JPEGLSCodec::JPEGLSCodec():BufferLength(0),Lossless(true),LossyError(0)
{
}

JPEGLSCodec::~JPEGLSCodec()
{
}

void JPEGLSCodec::SetLossless(bool l)
{
  Lossless = l;
}

bool JPEGLSCodec::GetLossless() const
{
  return Lossless;
}

bool JPEGLSCodec::GetHeaderInfo(std::istream &is, TransferSyntax &ts)
{
#ifndef GDCM_USE_JPEGLS
  return false;
#else
  is.seekg( 0, std::ios::end);
  std::streampos buf_size = is.tellg();
  char *dummy_buffer = new char[buf_size];
  is.seekg(0, std::ios::beg);
  is.read( dummy_buffer, buf_size);

  JlsParameters metadata;
  if (JpegLsReadHeader(dummy_buffer, buf_size, &metadata) != OK)
    {
    return false;
    }
  delete[] dummy_buffer;

  // $1 = {width = 512, height = 512, bitspersample = 8, components = 1, allowedlossyerror = 0, ilv = ILV_NONE, colorTransform = 0, custom = {MAXVAL = 0, T1 = 0, T2 = 0, T3 = 0, RESET = 0}}

  this->Dimensions[0] = metadata.width;
  this->Dimensions[1] = metadata.height;
  switch( metadata.bitspersample )
    {
  case 8:
    this->PF = PixelFormat( PixelFormat::UINT8 );
    break;
  case 12:
    this->PF = PixelFormat( PixelFormat::UINT16 );
    this->PF.SetBitsStored( 12 );
    break;
  case 16:
    this->PF = PixelFormat( PixelFormat::UINT16 );
    break;
  default:
    assert(0);
    }
  if( metadata.components == 1 )
    {
    PI = PhotometricInterpretation::MONOCHROME2;
    this->PF.SetSamplesPerPixel( 1 );
    }
  else if( metadata.components == 3 )
    {
    PI = PhotometricInterpretation::RGB;
    this->PF.SetSamplesPerPixel( 3 );
    }
  else assert(0);


  if( metadata.allowedlossyerror == 0 )
    {
    ts = TransferSyntax::JPEGLSLossless;
    }
  else
    {
    ts = TransferSyntax::JPEGLSNearLossless;
    }


  return true;
#endif
}

bool JPEGLSCodec::CanDecode(TransferSyntax const &ts) const
{
#ifndef GDCM_USE_JPEGLS
  return false;
#else
  return ts == TransferSyntax::JPEGLSLossless
      || ts == TransferSyntax::JPEGLSNearLossless;
#endif
}

bool JPEGLSCodec::CanCode(TransferSyntax const &ts) const
{
#ifndef GDCM_USE_JPEGLS
  return false;
#else
  return ts == TransferSyntax::JPEGLSLossless
      || ts == TransferSyntax::JPEGLSNearLossless;
#endif
}

bool JPEGLSCodec::Decode(DataElement const &in, DataElement &out)
{
#ifndef GDCM_USE_JPEGLS
  return false;
#else
  if( NumberOfDimensions == 2 )
    {
    const SequenceOfFragments *sf = in.GetSequenceOfFragments();
    assert( sf );
    std::stringstream is;
    unsigned long totalLen = sf->ComputeByteLength();
    char *buffer = new char[totalLen];
    sf->GetBuffer(buffer, totalLen);
    //is.write(buffer, totalLen);

    JlsParameters metadata;
    if (JpegLsReadHeader(buffer, totalLen, &metadata) != OK)
      {
      return false;
      }

    // allowedlossyerror == 0 => Lossless
    LossyFlag = metadata.allowedlossyerror;

    const BYTE* pbyteCompressed = (const BYTE*)buffer;
    int cbyteCompressed = totalLen;

    JlsParameters params;
    JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params);

    std::vector<BYTE> rgbyteCompressed;
    rgbyteCompressed.resize(params.height *params.width* 4);

    std::vector<BYTE> rgbyteOut;
    rgbyteOut.resize(params.height *params.width * ((params.bitspersample + 7) / 8) * params.components);

    JlsParameters theInfo;
    JLS_ERROR result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(),
      pbyteCompressed, cbyteCompressed, &theInfo);
    ASSERT(result == OK);
    (void)result;//warning removal

    delete[] buffer;

    out = in;

    VL::Type rgByteOutSize = (VL::Type)rgbyteOut.size();
    out.SetByteValue( (char*)&rgbyteOut[0], rgByteOutSize );
    return true;
    }
  else if( NumberOfDimensions == 3 )
    {
    const SequenceOfFragments *sf = in.GetSequenceOfFragments();
    assert( sf );
    assert( sf->GetNumberOfFragments() == Dimensions[2] );
    std::stringstream os;
    for(unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
      {
      const Fragment &frag = sf->GetFragment(i);
      if( frag.IsEmpty() ) return false;
      const ByteValue *bv = frag.GetByteValue();
      assert( bv );
      char *mybuffer = new char[bv->GetLength()];
    unsigned long totalLen = bv->GetLength();

      bv->GetBuffer(mybuffer, bv->GetLength());

    const BYTE* pbyteCompressed = (const BYTE*)mybuffer;
      while( totalLen > 0 && pbyteCompressed[totalLen-1] != 0xd9 )
        {
        totalLen--;
        }
  // what if 0xd9 is never found ?
  assert( totalLen > 0 && pbyteCompressed[totalLen-1] == 0xd9 );

    JlsParameters metadata;
    if (JpegLsReadHeader(mybuffer, totalLen, &metadata) != OK)
      {
      return false;
      }

    // allowedlossyerror == 0 => Lossless
    LossyFlag = metadata.allowedlossyerror;


    int cbyteCompressed = totalLen;

    JlsParameters params;
    JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params);

    std::vector<BYTE> rgbyteCompressed;
    rgbyteCompressed.resize(params.height *params.width* 4);

    std::vector<BYTE> rgbyteOut;
    rgbyteOut.resize(params.height *params.width * ((params.bitspersample + 7) / 8) * params.components);

    JlsParameters theInfo;
    JLS_ERROR result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(),
      pbyteCompressed, cbyteCompressed, &theInfo);
    ASSERT(result == OK);
    (void)result;//warning removal
bool r = true;


      delete[] mybuffer;
    os.write( (char*)&rgbyteOut[0], rgbyteOut.size() );

      if(!r) return false;
      assert( r == true );


      }
    std::string str = os.str();
    assert( str.size() );
    VL::Type strSize = (VL::Type)str.size();
    out.SetByteValue( &str[0], strSize );


    return true;
    }
    return false; 
	
#endif
}

//used by the streaming decoder
//therefore, takes in streaming parameters
//assumes that the entire data element for the pixels have been read in,
//and that they can be sequences or whatever.
bool JPEGLSCodec::Decode(DataElement const &in, char* outBuffer, uint32_t inBufferLength,
                         uint32_t inXMin, uint32_t inXMax, uint32_t inYMin,
                         uint32_t inYMax, uint32_t inZMin, uint32_t inZMax)
{
#ifndef GDCM_USE_JPEGLS
  return false;
#else
  if( NumberOfDimensions == 2 )
  {
    const SequenceOfFragments *sf = in.GetSequenceOfFragments();
    assert( sf );
    std::stringstream is;
    unsigned long totalLen = sf->ComputeByteLength();
    char *buffer = new char[totalLen];
    sf->GetBuffer(buffer, totalLen);
    //is.write(buffer, totalLen);
    
    JlsParameters metadata;
    if (JpegLsReadHeader(buffer, totalLen, &metadata) != OK)
    {
      return false;
    }
    
    // allowedlossyerror == 0 => Lossless
    LossyFlag = metadata.allowedlossyerror;
    
    const BYTE* pbyteCompressed = (const BYTE*)buffer;
    int cbyteCompressed = totalLen;
    
    JlsParameters params;
    JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params);
    
    std::vector<BYTE> rgbyteCompressed;
    rgbyteCompressed.resize(params.height *params.width* 4);
    
    //std::vector<BYTE> rgbyteOut;
    //rgbyteOut.resize(params.height *params.width * ((params.bitspersample + 7) / 8) * params.components);
    
    JlsParameters theInfo;
    JlsRect theROI;
    theROI.X = inXMin;
    theROI.Y = inYMin;
    theROI.Width = inXMax - inXMin;
    theROI.Height = inYMax - inYMin;
    JLS_ERROR result = JpegLsDecodeRect(outBuffer, inBufferLength, 
                                        pbyteCompressed, cbyteCompressed, 
                                        theROI, &theInfo);
    
    //JLS_ERROR result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(),
    //                                pbyteCompressed, cbyteCompressed, &theInfo);
    //ASSERT(result == OK);
    //(void)result;//warning removal
    
    delete[] buffer;
    
    //out = in;
    
    //VL::Type rgByteOutSize = (VL::Type)rgbyteOut.size();
    //out.SetByteValue( (char*)&rgbyteOut[0], rgByteOutSize );
    return result == OK;
  }
  else if( NumberOfDimensions == 3 )
  {
    return false; //no 3D support yet
    /*
    const SequenceOfFragments *sf = in.GetSequenceOfFragments();
    assert( sf );
    assert( sf->GetNumberOfFragments() == Dimensions[2] );
    std::stringstream os;
    for(unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
    {
      const Fragment &frag = sf->GetFragment(i);
      if( frag.IsEmpty() ) return false;
      const ByteValue *bv = frag.GetByteValue();
      assert( bv );
      char *mybuffer = new char[bv->GetLength()];
      unsigned long totalLen = bv->GetLength();
      
      bv->GetBuffer(mybuffer, bv->GetLength());
      
      const BYTE* pbyteCompressed = (const BYTE*)mybuffer;
      while( totalLen > 0 && pbyteCompressed[totalLen-1] != 0xd9 )
      {
        totalLen--;
      }
      // what if 0xd9 is never found ?
      assert( totalLen > 0 && pbyteCompressed[totalLen-1] == 0xd9 );
      
      JlsParameters metadata;
      if (JpegLsReadHeader(mybuffer, totalLen, &metadata) != OK)
      {
        return false;
      }
      
      // allowedlossyerror == 0 => Lossless
      LossyFlag = metadata.allowedlossyerror;
      
      
      int cbyteCompressed = totalLen;
      
      JlsParameters params;
      JpegLsReadHeader(pbyteCompressed, cbyteCompressed, &params);
      
      std::vector<BYTE> rgbyteCompressed;
      rgbyteCompressed.resize(params.height *params.width* 4);
      
      std::vector<BYTE> rgbyteOut;
      rgbyteOut.resize(params.height *params.width * ((params.bitspersample + 7) / 8) * params.components);
      
      JlsParameters theInfo;
      JLS_ERROR result = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(),
                                      pbyteCompressed, cbyteCompressed, &theInfo);
      ASSERT(result == OK);
      (void)result;//warning removal
      bool r = true;
      
      
      delete[] mybuffer;
      os.write( (char*)&rgbyteOut[0], rgbyteOut.size() );
      
      if(!r) return false;
      assert( r == true );
      
      
    }
    std::string str = os.str();
    assert( str.size() );
    VL::Type strSize = (VL::Type)str.size();
    out.SetByteValue( &str[0], strSize );
    
    
    return true;
    */
  }
  return false;
  
#endif
}

// Compress into JPEG
bool JPEGLSCodec::Code(DataElement const &in, DataElement &out)
{
#ifndef GDCM_USE_JPEGLS
  return false;
#else
  out = in;
  //
  // Create a Sequence Of Fragments:
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const Tag itemStart(0xfffe, 0xe000);
  //sq->GetTable().SetTag( itemStart );

  const unsigned int *dims = this->GetDimensions();
  int image_width = dims[0];
  int image_height = dims[1];
  const PixelFormat &pf = this->GetPixelFormat();
  int sample_pixel = pf.GetSamplesPerPixel();
  int bitsallocated = pf.GetBitsAllocated();
  int bitsstored = pf.GetBitsStored();

  const ByteValue *bv = in.GetByteValue();
  const char *input = bv->GetPointer();
  unsigned long len = bv->GetLength();
  unsigned long image_len = len / dims[2];
  size_t inputlength = image_len;

  for(unsigned int dim = 0; dim < dims[2]; ++dim)
    {
    const char *inputdata = input + dim * image_len; //bv->GetPointer();

    JlsParameters params;
    memset( &params, 0, sizeof(params) );
/*
The fields in JlsCustomParameters do not control lossy/lossless. They
provide the possiblity to tune the JPEG-LS internals for better compression
ratios. Expect a lot of work and testing to achieve small improvements.

Lossy/lossless is controlled by the field allowedlossyerror. If you put in
0, encoding is lossless. If it is non-zero, then encoding is lossy. The
value of 3 is often suggested as a default.

The nice part about JPEG-LS encoding is that in lossy encoding, there is a
guarenteed maximum error for each pixel. So a pixel that has value 12,
encoded with a maximum lossy error of 3, may be decoded as a value between 9
and 15, but never anything else. In medical imaging this could be a useful
guarantee.

The not so nice part is that you may see striping artifacts when decoding
"non-natural" images. I haven't seen the effects myself on medical images,
but I suspect screenshots may suffer the most. Also, the bandwidth saving is
not as big as with other lossy schemes.

As for 12 bit, I am about to commit a unit test (with the sample you gave
me) that does a successful round trip encoding of 12 bit color. I did notice
that for 12 bit, the encoder fails if the unused bits are non-zero, but the
sample dit not suffer from that.
*/
    params.allowedlossyerror = Lossless ? 0 : LossyError;
    params.components = sample_pixel;
    // D_CLUNIE_RG3_JPLY.dcm. The famous 16bits allocated / 10 bits stored with the pixel value = 1024
    // CharLS properly encode 1024 considering it as 10bits data, so the output
    // Using bitsstored for the encoder gives a slightly better compression ratio, and is indeed the
    // right way of doing it.

/*
FIXME: 12bits RGB is not working, so for now use the bitsallocated all the time
Problem reading image from: /home/mathieu/Creatis/gdcmData/SIEMENS-MR-RGB-16Bits.dcm
Found 8fd82891d8c7f146656aa88160c69b0b instead of faff9970b905458c0844400b5b869e25
*/
    if( true || pf.GetPixelRepresentation() )
      {
      // gdcmData/CT_16b_signed-UsedBits13.dcm
      params.bitspersample = bitsallocated;
      }
    else
      {
      params.bitspersample = bitsstored;
      }
    params.height = image_height;
    params.width = image_width;

    if (sample_pixel == 4)
      {
      params.ilv = ILV_LINE;
      }
    else if (sample_pixel == 3)
      {
      params.ilv = ILV_LINE;
      params.colorTransform = COLORXFORM_HP1;
      }

    std::vector<BYTE> rgbyteCompressed;
    rgbyteCompressed.resize(image_width * image_height * 4);
    //rgbyteCompressed.resize(size.cx *size.cy * ccomp * cbit / 4);

    size_t cbyteCompressed;
    JLS_ERROR error = JpegLsEncode(&rgbyteCompressed[0], rgbyteCompressed.size(),
      &cbyteCompressed, inputdata, inputlength, &params);
    if( error != OK )
      {
      gdcmErrorMacro( "Error compressing: " << error );
      return false;
      }

    assert( cbyteCompressed < rgbyteCompressed.size() );

    Fragment frag;
    VL::Type cbyteCompressedLen = (VL::Type)cbyteCompressed;
    frag.SetByteValue( (char*)&rgbyteCompressed[0], cbyteCompressedLen );
    sq->AddFragment( frag );
    }

  assert( sq->GetNumberOfFragments() == dims[2] );
  out.SetValue( *sq );

  return true;

#endif
}

void JPEGLSCodec::SetLossyError(int error)
{
  LossyError = error;
}

} // end namespace gdcm
