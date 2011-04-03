/*
 * ROP_NaiadCam.C
 *
 * Houdini Buddy - Naiad camera converter
 *
 * Copyright (c) 2010 Van Aarde Krynauw.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __ROP_NAIAD_CAM_H__
#define __ROP_NAIAD_CAM_H__

#include <ROP/ROP_Node.h>
#include <SOP_UI_Macros.h>

class OP_TemplatePair;
class OP_VariablePair;

//TODO: Add parameters to control the output EMP timestamp

#define PRM_FILENAME "file"
#define PRM_CAMERA "camera"
#define PRM_BODYNAME "bodyname"

namespace HoudiniBuddy 
{
	static const std::string PRM_FileName;

	class ROP_NaiadCam : public ROP_Node 
	{
		public:
			static PRM_Template myTemplateList[];

			/// Provides access to our parm templates.
			static OP_TemplatePair	*getTemplatePair();
			/// Provides access to our variables.
			static OP_VariablePair	*getVariablePair();
			/// Creates an instance of this node.
			static OP_Node		*myConstructor(OP_Network *net, const char*name, OP_Operator *op);
		protected:
			ROP_NaiadCam(OP_Network *net, const char *name, OP_Operator *entry);
			virtual ~ROP_NaiadCam();

			/// Called at the beginning of rendering to perform any intialization 
			/// necessary.
			/// @param	nframes	    Number of frames being rendered.
			/// @param	s	    Start time, in seconds.
			/// @param	e	    End time, in seconds.
			/// @return		    True of success, false on failure (aborts the render).
			virtual int			 startRender(int nframes, float s, float e);

			/// Called once for every frame that is rendered.
			/// @param	time	    The time to render at.
			/// @param	boss	    Interrupt handler.
			/// @return		    Return a status code indicating whether to abort the
			///			    render, continue, or retry the current frame.
			virtual ROP_RENDER_CODE	 renderFrame(float time, UT_Interrupt *boss);

			/// Called after the rendering is done to perform any post-rendering steps
			/// required.
			/// @return		    Return a status code indicating whether to abort the
			///			    render, continue, or retry.
			virtual ROP_RENDER_CODE	 endRender();

			public:

			/// A convenience method to evaluate our custom file parameter.
			//void  OUTPUT(UT_String &str, float t)
			//{ STR_PARM("file",  0, 0, t) }

			GET_PARM_S(PRM_FILENAME, OutputFile);
			GET_PARM_S(PRM_CAMERA, CameraNode);
			GET_PARM_S(PRM_BODYNAME, BodyName);

		private:
			static int		*ifdIndirect;
			float		 myEndTime;
	};

}

#endif

