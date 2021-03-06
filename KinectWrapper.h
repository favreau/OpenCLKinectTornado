/* Copyright (c) 2011-2012, Cyrille Favreau
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 * This file is part of OpenCLKinectTornado
 * <https://github.com/favreau/OpenCLKinectTornado>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <mediaobj.h>
#include <nuiapi.h>
#include <windows.h>

struct float3 {
  float x, y, z;
};

class KinectWrapper {
public:
  static const int gKinectDepthDepth = 2;
  static const int gDepthWidth = 320;
  static const int gDepthHeight = 240;

  static const int gKinectVideoDepth = 4;
  static const int gVideoWidth = 640;
  static const int gVideoHeight = 480;

public:
  KinectWrapper(void);
  ~KinectWrapper(void);

public:
  void initialize();
  BYTE *getDepthFrame();
  BYTE *getVideoFrame();
  bool getSkeletonPosisions(float3 *positions);

public:
  bool getClick();

public:
  void DepthToWorld(int x, int y, int depthValue, float &rx, float &ry,
                    float &rz);
  float RawDepthToMeters(int depthValue);

  RGBQUAD KinNuiShortToQuadDepth(USHORT s);

private:
  int m_mouse_x;
  int m_mouse_y;

  HANDLE m_skeletons;
  HANDLE m_hNextDepthFrameEvent;
  HANDLE m_hNextVideoFrameEvent;
  HANDLE m_hNextSkeletonEvent;
  HANDLE m_pVideoStreamHandle;
  HANDLE m_pDepthStreamHandle;
  NUI_SKELETON_FRAME m_skeletonFrame;

private:
  float3 m_lastRightHandPosition;
  INuiAudioBeam *m_pAudio;
};

class CStaticMediaBuffer : public IMediaBuffer {
public:
  CStaticMediaBuffer() {}
  CStaticMediaBuffer(BYTE *pData, ULONG ulSize, ULONG ulData)
      : m_pData(pData), m_ulSize(ulSize), m_ulData(ulData), m_cRef(1) {}
  STDMETHODIMP_(ULONG) AddRef() { return 2; }
  STDMETHODIMP_(ULONG) Release() { return 1; }
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
    if (riid == IID_IUnknown) {
      AddRef();
      *ppv = (IUnknown *)this;
      return NOERROR;
    } else if (riid == IID_IMediaBuffer) {
      AddRef();
      *ppv = (IMediaBuffer *)this;
      return NOERROR;
    } else
      return E_NOINTERFACE;
  }
  STDMETHODIMP SetLength(DWORD ulLength) {
    m_ulData = ulLength;
    return NOERROR;
  }
  STDMETHODIMP GetMaxLength(DWORD *pcbMaxLength) {
    *pcbMaxLength = m_ulSize;
    return NOERROR;
  }
  STDMETHODIMP GetBufferAndLength(BYTE **ppBuffer, DWORD *pcbLength) {
    if (ppBuffer)
      *ppBuffer = m_pData;
    if (pcbLength)
      *pcbLength = m_ulData;
    return NOERROR;
  }
  void Init(BYTE *pData, ULONG ulSize, ULONG ulData) {
    m_pData = pData;
    m_ulSize = ulSize;
    m_ulData = ulData;
  }

protected:
  BYTE *m_pData;
  ULONG m_ulSize;
  ULONG m_ulData;
  ULONG m_cRef;
};
