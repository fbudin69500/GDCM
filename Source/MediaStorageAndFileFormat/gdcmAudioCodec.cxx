/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmAudioCodec.h"
#include "gdcmDataElement.h"

namespace gdcm
{

AudioCodec::AudioCodec()
{
}

AudioCodec::~AudioCodec()
{
}

bool AudioCodec::Decode(DataElement const &is, DataElement &os)
{
  os = is;
  return true;
}

}
