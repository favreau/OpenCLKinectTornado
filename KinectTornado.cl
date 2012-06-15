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

float RawDepthToMeters(int depthValue)
{
   if (depthValue < 2047)
   {
      return (float)(0.1f / ((float)depthValue * -0.0030711016 + 3.3309495161));
   }
   return 0.f;
}

__kernel void sine_wave(
   __global float4* pos, 
   unsigned int width, 
   unsigned int height, 
   float time,
   __global char* depth)
{
   unsigned int x = get_global_id(0);
   unsigned int y = get_global_id(1);

   // calculate uv coordinates
   float u = x / (float) width;
   float v = y / (float) height;

   u = u*2.f - 1.f;
   v = v*2.f - 1.f;

   // kinect depth
   int index = 2*(y*width+x);

   int a = (char)depth[index];
   short player = a & 7;
   a = a & 0xF8;
   a = a >> 3;
   int b = (char)depth[index+1];
   b = b << 8;

   float w = 100.f;
   if( player != 0 ) 
   {
      int c = b|a;
      w = c/10000.f;
   }

   // calculate simple sine wave pattern
   //float w = sin(u*freq + time) * cos(v*freq + time) * 0.5f; // + (rawDepth*0.00005f);
 // + sin(u*freq+time) * cos(v*freq+time) * 0.5f; // + (rawDepth*0.00005f);

   // write output vertex
   pos[y*width+x] = (float4)( u, -v, 2.f-w*w, 1.f );
}

