#include <iostream>
#include <stdio.h>
#include <devicetopology.h>

#include "KinectWrapper.h"

KinectWrapper::KinectWrapper(void)
 : m_skeletons(0), 
   m_hNextDepthFrameEvent(0), 
   m_hNextVideoFrameEvent(0), 
   m_hNextSkeletonEvent(0),
   m_pVideoStreamHandle(0), 
   m_pDepthStreamHandle(0),
   m_mouse_x(0),
   m_mouse_y(0),
   m_pAudio(nullptr)
{
   m_lastRightHandPosition.x = 0.f;
   m_lastRightHandPosition.y = 0.f;
   m_lastRightHandPosition.z = 0.f;
}


KinectWrapper::~KinectWrapper(void)
{
   CloseHandle(m_skeletons);
   CloseHandle(m_hNextDepthFrameEvent); 
   CloseHandle(m_hNextVideoFrameEvent); 
   CloseHandle(m_hNextSkeletonEvent);
   NuiShutdown();

   m_skeletons=0;
   m_hNextDepthFrameEvent=0;
   m_hNextVideoFrameEvent=0;
   m_hNextSkeletonEvent=0;
   m_pVideoStreamHandle=0;
   m_pDepthStreamHandle=0;
}

void KinectWrapper::initialize()
{
   // Initialize Kinect
   int status = NuiInitialize( 
      NUI_INITIALIZE_FLAG_USES_AUDIO | 
      NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | 
      NUI_INITIALIZE_FLAG_USES_SKELETON | 
      NUI_INITIALIZE_FLAG_USES_COLOR);

   m_hNextDepthFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); 
   m_hNextVideoFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); 
   m_hNextSkeletonEvent   = CreateEvent( NULL, TRUE, FALSE, NULL );

   m_skeletons = CreateEvent( NULL, TRUE, FALSE, NULL );			 
   status = NuiSkeletonTrackingEnable( m_skeletons, 0 );

   status = NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, m_hNextVideoFrameEvent, &m_pVideoStreamHandle );
   status = NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_320x240, 0, 2, m_hNextDepthFrameEvent, &m_pDepthStreamHandle );
   //status = NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_320x240, 0, 2, m_hNextDepthFrameEvent, &m_pDepthStreamHandle );

   status = NuiCameraElevationSetAngle( 3 );
}

BYTE* KinectWrapper::getDepthFrame()
{
   BYTE* depth(0);

   // Depth
   const NUI_IMAGE_FRAME* pDepthFrame = 0;
   WaitForSingleObject (m_hNextDepthFrameEvent,INFINITE); 
   int status = NuiImageStreamGetNextFrame( m_pDepthStreamHandle, 0, &pDepthFrame ); 
   if(( status == S_OK) && pDepthFrame ) 
   {
      INuiFrameTexture* pTexture = pDepthFrame->pFrameTexture;
      if( pTexture ) 
      {
         NUI_LOCKED_RECT LockedRectDepth;
         pTexture->LockRect( 0, &LockedRectDepth, NULL, 0 ) ; 
         if( LockedRectDepth.Pitch != 0 ) 
         {
            depth = (BYTE*) LockedRectDepth.pBits;
         }
      }
   }
   NuiImageStreamReleaseFrame( m_pDepthStreamHandle, pDepthFrame ); 
   return depth;
}

BYTE* KinectWrapper::getVideoFrame()
{
   BYTE* video(0);
   // Video
   const NUI_IMAGE_FRAME* pImageFrame = 0;
   WaitForSingleObject (m_hNextVideoFrameEvent,INFINITE); 
   int status = NuiImageStreamGetNextFrame( m_pVideoStreamHandle, 0, &pImageFrame ); 
   if(( status == S_OK) && pImageFrame ) 
   {
      INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
      NUI_LOCKED_RECT LockedRect;
      pTexture->LockRect( 0, &LockedRect, NULL, 0 ) ; 
      if( LockedRect.Pitch != 0 ) 
      {
         video = (BYTE*) LockedRect.pBits;
      }
   }
   NuiImageStreamReleaseFrame( m_pVideoStreamHandle, pImageFrame ); 
   return video;
}

bool KinectWrapper::getSkeletonPosisions(float3* positions)
{
   bool found = false;
   HRESULT hr = NuiSkeletonGetNextFrame( 0, &m_skeletonFrame );
   int i=0;
   while( i<NUI_SKELETON_COUNT && !found ) 
   {
      if( m_skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED ) 
      {
         for( int j=0; j<NUI_SKELETON_POSITION_COUNT; j++ )
         {
            positions[j].x = m_skeletonFrame.SkeletonData[i].SkeletonPositions[j].x;
            positions[j].y = m_skeletonFrame.SkeletonData[i].SkeletonPositions[j].y;
            positions[j].z = m_skeletonFrame.SkeletonData[i].SkeletonPositions[j].z;

            if( j == NUI_SKELETON_POSITION_HAND_RIGHT )
            {
               m_lastRightHandPosition = positions[j];
            }
         }
         found = true;
      }
      i++;
   }
   return found;
}

float KinectWrapper::RawDepthToMeters(int depthValue)
{
   if (depthValue < 2047)
   {
      return float(1.0 / (double(depthValue) * -0.0030711016 + 3.3309495161));
   }
   return 0.0f;
}

void KinectWrapper::DepthToWorld(int x, int y, int depthValue, float& rx, float& ry, float& rz)
{
   static const double fx_d = 1.0 / 5.9421434211923247e+02;
   static const double fy_d = 1.0 / 5.9104053696870778e+02;
   static const double cx_d = 3.3930780975300314e+02;
   static const double cy_d = 2.4273913761751615e+02;

   const double depth = RawDepthToMeters(depthValue);
   rx = float((x - cx_d) * depth * fx_d);
   ry = float((y - cy_d) * depth * fy_d);
   rz = float(depth);
}


RGBQUAD KinectWrapper::KinNuiShortToQuadDepth( USHORT s ) 
{ 
   //Décale de 3 Bits vers la droite afin de ne récupère que les informations

   //concernant la profondeur

   USHORT RealDepth = (s & 0xfff8) >> 3; 
   //Masque pour retrouver si un player a été detecté

   USHORT Player = s & 7; 
   // Transforme les informations de profondeur sur 13-Bit en une intensité codé sur 8 Bits 

   // afin d'afficher une couleur en fonction de la profondeur

   BYTE l = 255 - (BYTE)(256*RealDepth/0x0fff); 
   RGBQUAD q; 
   q.rgbRed = q.rgbBlue = q.rgbGreen = 0; 
   switch( Player ) 
   { 
   case 0: 
      //Affiche les informations (non player) en niveau de gris 
      q.rgbRed = l / 2; 
      q.rgbBlue = l / 2; 
      q.rgbGreen = l / 2; 
      break; 
      //Si un player est detecté affiche le en couleur 
      //avec une intensité relatif à la profondeur 
   case 1: 
      q.rgbRed = l; 
      break; 
   case 2: 
      q.rgbGreen = l; 
      break; 
   case 3: 
      q.rgbRed = l / 4; 
      q.rgbGreen = l; 
      q.rgbBlue = l; 
      break; 
   case 4: 
      q.rgbRed = l; 
      q.rgbGreen = l; 
      q.rgbBlue = l / 4; 
      break; 
   case 5: 
      q.rgbRed = l; 
      q.rgbGreen = l / 4; 
      q.rgbBlue = l; 
      break; 
   case 6: 
      q.rgbRed = l / 2; 
      q.rgbGreen = l / 2; 
      q.rgbBlue = l; 
      break; 
   case 7: 
      q.rgbRed = 255 - ( l / 2 ); 
      q.rgbGreen = 255 - ( l / 2 ); 
      q.rgbBlue = 255 - ( l / 2 ); 
   } 
   return q; 
}

bool KinectWrapper::getClick()
{
   bool returnValue(false);
#if 0
   if( m_pAudio == nullptr )
   {
      NuiGetAudioSource( &m_pAudio );
   }

   IMediaObject* pDMO;
   m_pAudio->QueryInterface(IID_IMediaObject, (void**)&pDMO);

   DWORD status(0);

   // allocate output buffer
   WAVEFORMATEX wfxOut = {WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, 0};
   DWORD cOutputBufLen = wfxOut.nSamplesPerSec * wfxOut.nBlockAlign;
   BYTE *pbOutputBuffer = nullptr;
   pbOutputBuffer = new BYTE[cOutputBufLen];

   CStaticMediaBuffer outputBuffer;
   outputBuffer.Init((byte*)pbOutputBuffer, cOutputBufLen, 0);

   DMO_OUTPUT_DATA_BUFFER OutputBufferStruct = {0};
   OutputBufferStruct.pBuffer = &outputBuffer;

   pDMO->ProcessOutput(DMO_PROCESS_OUTPUT_DISCARD_WHEN_NO_BUFFER, 1, &OutputBufferStruct, &status );

   double angle;
   double confidence;
   m_pAudio->GetPosition( &angle, &confidence );
   
   std::cout << "Angle=" << angle << ", Confidence=" << confidence << std::endl;

   if( confidence > 0.5f )
   {
      double rightHandAngle = asin(m_lastRightHandPosition.x/m_lastRightHandPosition.z);
      returnValue = (abs(angle - rightHandAngle) < 0.5f);
   }

   delete pbOutputBuffer;
#endif // 0
   return returnValue;
}
