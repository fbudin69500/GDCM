/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __gdcmMediaStorage_h
#define __gdcmMediaStorage_h

#include "gdcmTransferSyntax.h"

namespace gdcm
{

/*
 * FIXME There should not be any notion of Image and/or PDF at that point
 * Only the codec can answer yes I support this Media Storage or not...
 * For instance an ImageCodec will answer yes to most of them
 * while a PDFCodec will answer only for the Encaplusated PDF
 */
class DataSet;
class MediaStorage
{
public:
  typedef enum {
    MediaStorageDirectoryStorage,
    ComputedRadiographyImageStorage,
    DigitalXRayImageStorageForPresentation,
    DigitalXRayImageStorageForProcessing,
    DigitalMammographyImageStorageForPresentation,
    DigitalMammographyImageStorageForProcessing,
    DigitalIntraoralXrayImageStorageForPresentation,
    DigitalIntraoralXRayImageStorageForProcessing,
    CTImageStorage,
    EnhancedCTImageStorage,
    UltrasoundMultiFrameImageStorageRetired,
    UltrasoundMultiFrameImageStorage,
    MRImageStorage,
    EnhancedMRImageStorage,
    MRSpectroscopyStorage,
    NuclearMedicineImageStorageRetired,
    UltrasoundImageStorageRetired,
    UltrasoundImageStorage,
    SecondaryCaptureImageStorage,
    MultiframeSingleBitSecondaryCaptureImageStorage,
    MultiframeGrayscaleByteSecondaryCaptureImageStorage,
    MultiframeGrayscaleWordSecondaryCaptureImageStorage,
    MultiframeTrueColorSecondaryCaptureImageStorage,
    StandaloneOverlayStorage,
    StandaloneCurveStorage,
    LeadECGWaveformStorage, // 12-
    GeneralECGWaveformStorage,
    AmbulatoryECGWaveformStorage,
    HemodynamicWaveformStorage,
    CardiacElectrophysiologyWaveformStorage,
    BasicVoiceAudioWaveformStorage,
    StandaloneModalityLUTStorage,
    StandaloneVOILUTStorage,
    GrayscaleSoftcopyPresentationStateStorageSOPClass,
    XRayAngiographicImageStorage,
    XRayRadiofluoroscopingImageStorage,
    XRayAngiographicBiPlaneImageStorageRetired,
    NuclearMedicineImageStorage,
    RawDataStorage,
    SpacialRegistrationStorage,
    SpacialFiducialsStorage,
    PETImageStorage,
    RTImageStorage,
    RTDoseStorage,
    RTStructureSetStorage,
    RTPlanStorage,
    CSANonImageStorage,
    Philips3D,
    MS_END
  } MSType; // Media Storage Type

typedef enum {
	  NoObject = 0, // DICOMDIR
	  Video, // Most common, include image, video and volume
	  Waveform, // Isn't it simply a 1D video ?
	  Audio, // ???
	  PDF,
	  URI, // URL...
	  Segmentation, // TODO
	  ObjectEnd
  } ObjectType;

  static const char* GetMSString(MSType ts);
  static const MSType GetMSType(const char *str);

  MediaStorage(MSType type = MS_END):MSField(type) {}

  static bool IsImage(const MSType &ts);

  operator MSType () const { return MSField; }

  const char *GetModality() const;

  void SetFromDataSet(DataSet const &ds);

  friend std::ostream &operator<<(std::ostream &os, const MediaStorage &ms);
private:
  MSType MSField;
};
//-----------------------------------------------------------------------------
inline std::ostream &operator<<(std::ostream &_os, const MediaStorage &ms)
{
  _os << MediaStorage::GetMSString(ms);
  return _os;

}

} // end namespace gdcm

#endif // __gdcmMediaStorage_h
