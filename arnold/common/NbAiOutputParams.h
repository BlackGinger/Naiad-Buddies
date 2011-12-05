// ----------------------------------------------------------------------------
//
// NbAiOutputParams.h
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of Exotic Matter AB nor its contributors may be used to
//   endorse or promote products derived from this software without specific
//   prior written permission.
//
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT
//    LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
//    FOR  A  PARTICULAR  PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL THE
//    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//    BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS  OR  SERVICES;
//    LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER
//    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT
//    LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
//    ANY  WAY OUT OF THE USE OF  THIS SOFTWARE,  EVEN IF ADVISED OF  THE
//    POSSIBILITY OF SUCH DAMAGE.
//
// ----------------------------------------------------------------------------

#ifndef NBAIOUTPUTPARAMS_H_
#define NBAIOUTPUTPARAMS_H_

namespace NbAi{

class OutputParams
{
public:
    OutputParams(const Nb::String & implicitShader,
                 const Nb::String &       assScene,
                 const Nb::String &    imageFormat,
                 const Nb::String &    outputImage,
                 const int                   width,
                 const int                  height,
                 const int                 threads,
                 const float            motionBlur,
                 const int                     fps,
                 const int                 padding,
                 const Nb::String        outputAss = Nb::String(""),
                 const Nb::String          kickAss = Nb::String(""),
                 const Nb::String          geoProc = Nb::String("")
                ):
                     _implicitShader(implicitShader),
                     _assScene(assScene),
                     _imageFormat(_ArnoldImageFormat(imageFormat)),
                     _outputImage(outputImage),
                     _width(width),
                     _height(height),
                     _threads(threads),
                     _motionBlur(motionBlur),
                     _fps(fps),
                     _padding(padding),
                     _timePerFrame( _motionBlur != 0 ? 1.f / _fps : 0.f),
                     _outputAss(outputAss),
                     _kickAss(kickAss),
                     _geoProc(geoProc)
    {
        //
    };
// ----------------------------------------------------------------------------
    const char * getImplicitShader() const { return _implicitShader.c_str();};
    const char * getAssScene()       const { return _assScene.c_str();};
    const char * getImageFormat()    const { return _imageFormat.c_str();};
    const char * getOutputImage()    const { return _outputImage.c_str();};
    int          getWidth()          const { return _width;};
    int          getHeight()         const { return _height;};
    int          getThreads()        const { return _threads;};
    float        getMotionBlur()     const { return _motionBlur;};
    int          getFPS()            const { return _fps;};
    int          getPadding()        const { return _padding;};
    float        getTimePerFrame()   const { return _timePerFrame;};
    const char * getOutputAss()      const { return _outputAss.c_str();};
    const char * getKickAss()        const { return _kickAss.c_str();};
    const char * getGeoProc()        const { return _geoProc.c_str();};
// ----------------------------------------------------------------------------
    friend std::ostream &
    operator << (std::ostream& os,const OutputParams& p)
    {
        return os << "\tImplicit Shader: " << p._implicitShader << "\n"
                  << "\tAss Scene: " << p._assScene << "\n"
                  << "\tImage Format: " << p._imageFormat << "\n"
                  << "\tOutput Image: " << p._outputImage << "\n"
                  << "\tWidth: " << p._width << "\n"
                  << "\tHeight: " << p._height << "\n"
                  << "\tThreads: " << p._threads << "\n"
                  << "\tMotion Blur: " << p._motionBlur << "\n"
                  << "\tFPS: " << p._fps << "\n"
                  << "\tPadding: " << p._padding << "\n"
                  << "\tTime Per Frame: " << p._timePerFrame << "\n"
                  << "\tOutput Ass: " << p._outputAss << "\n"
                  << "\tKick Ass: " << p._kickAss << "\n"
                  << "\tGeo Procedural: " << p._geoProc << std::endl;
    };
// ----------------------------------------------------------------------------
private:
    const Nb::String _implicitShader;
    const Nb::String       _assScene;
    const Nb::String    _imageFormat;
    const Nb::String    _outputImage;

    const int                 _width;
    const int                _height;
    const int               _threads;
    const float          _motionBlur;
    const int                   _fps;
    const int               _padding;
    const float        _timePerFrame;

    //Only exist in Ass Op's
    const Nb::String      _outputAss;
    const Nb::String        _kickAss;
    const Nb::String        _geoProc;
// ----------------------------------------------------------------------------
    Nb::String
    _ArnoldImageFormat(const Nb::String & s) const
    {
        return Nb::String("driver_") + s;
    };

};

}//end namespace
#endif /* NBAIOUTPUTPARAMS_H_ */
