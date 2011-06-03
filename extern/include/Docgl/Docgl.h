/* -------------------------------- . ---------------------------------------- .
| Filename : Docgl.h                | DOCGL3.3                                 |
| Author   : Alexandre Buge         | D-LABS Object Close Graphic Library 3.3  |
| Started  : 25/01/2011 10:09       | Simple C++ OpenGL3.3 wrapper             |
` --------------------------------- . ---------------------------------------- |
|                                                                              |
| Structured strategy                                                          |
| - DOCGL classes kind are Contexts, Registers and Objects.                    |
| - Registers wrap OpenGL state values.                                        |
| - Contexts are Registers store.                                              |
| - Objects host OpenGL name id and can have Registers (or Properties).        |
| - Registers and Objects have setXXX and getXXX methods kind.                 |
| - Objects have createXXX destroy linkXXX and setId methods.                  |
|                                                                              |
| Lifetime strategy:                                                           |
| - Contexts are global and must be unique per OpenGL context.                 |
| - Contexts calls in Registers Construtors are forbidden.                     |
| - Objects must be deleted before their referring Contexts.                   |
| - Objects'ids are modified by createXXX destroy and setId methods.           |
| - Objects caller are responsible to call createXXX, destroy and setId.       |
| - Objects provide only debug assertion to prevent potential VRAM leak.       |
|                                                                              |
| Side effect strategy:                                                        |
| - No more OpenGlobaL C-like functions.                                       |
| - Objects methods ensure to restore Contexts Registers values before leave.  |
| - Objects can use GLScopedSetValue to restore Contexts Registers values.     |
| - GLScopedSetValue produce perforance penalty on non cached Registers.       |
|                                                                              |
| Error strategy:                                                              |
| - All methods assert on bad in/out values and return GLErrorFlags.           |
| - createXXX and getXXX return OpenGL error with potential perforance penalty.|
| - setXXX and linkXXX return flags WITHOUT OpenGL call perforance penalty.    |
| - jassertglsucceed must follow OpenGL calls without popContextErrorFlags.    |
` --------------------------------------------------------------------------- */

#ifndef DOCGL_H_
# define DOCGL_H_

# ifdef WIN32
#  include <gl/glew.h>			// OpenGL Extension "autoloader"
# endif // !WIN32

# ifdef __APPLE__
#  include <stdlib.h>
#  include <TargetConditionals.h>
#  include <GL/glew.h>
#  include <OpenGL/gl.h>		// Apple OpenGL haders (version depends on OS X SDK version)
# endif // !__APPLE__

# ifdef linux
#  include <GL/glew.h>
#  include <csignal> /* for sigtrap */
#  include <cstring> /** for memcpy*/
# endif // !linux

# if defined(DEBUG) || defined(_DEBUG)
#  ifndef jassertfalse
#   ifdef WIN32
#    ifdef _MSC_VER
#     define jassertfalse __debugbreak()
#    elif __GNUC__
#     define jassertfalse asm("int $3");
#    else  // !_MSC_VER
#     define jassertfalse __asm int 3
#    endif // !_MSC_VER
#   elif __APPLE__
#     define jassertfalse Debugger();
#   elif linux
#     define jassertfalse kill (0, SIGTRAP);
#   endif // !WIN32
#  endif // jassertfalse
#  ifndef jassert
#   define jassert(X) {if (!(X)) jassertfalse;}
#  endif // !jassert
# else // !DEBUG
#  define jassert(X)
#  define jassertfalse
# endif // !DEBUG

# include <map> // for GLIntegerConstantsStore

namespace docgl
{

//////////////////////////////////////////////////////////////////////////////

class GLErrorFlags
{
public:
  // construction
  GLErrorFlags()
    : errorFlags(0), unknownErrorCode(GL_NO_ERROR) {}
  GLErrorFlags(const GLErrorFlags& other)
  {
    errorFlags = other.errorFlags;
    unknownErrorCode = other.unknownErrorCode;
  }
  // custom error generation constructor
  enum ErrorFlags
  {
    invalidEnumFlag                 = 1,
    invalidValueFlag                = 2,
    invalidOperationFlag            = 4,
    outOfVideoMemoryFlag            = 8,
    invalidFrameBufferOperationFlag = 16,
  };
  GLErrorFlags(ErrorFlags errorFlags)
  {
    unknownErrorCode = 0;
    this->errorFlags = errorFlags;
  }

  void merge(const GLErrorFlags& otherErrorFlags)
  {
    errorFlags |= otherErrorFlags.errorFlags;
    if (otherErrorFlags.unknownErrorCode != GL_NO_ERROR)
    {
      jassert(unknownErrorCode == GL_NO_ERROR); // Impossible merge on unknownErrorCode: loosing previous value
      unknownErrorCode = otherErrorFlags.unknownErrorCode;
    }
  }

  // useful value
  static const GLErrorFlags succeed;

  // flags check
  bool hasErrors() const
    {return errorFlags != 0 || unknownErrorCode != GL_NO_ERROR;}
  bool hasSucceed() const
    {return errorFlags == 0 && unknownErrorCode == GL_NO_ERROR;}

  // fine flags check
  bool hasInvalidEnum() const
    {return errorFlags & invalidEnumFlag;}
  bool hasInvalidValue() const
    {return errorFlags & invalidValueFlag ? true : false;}
  bool hasInvalidOperation() const
    {return errorFlags & invalidOperationFlag ? true : false;}
  bool hasOutOfMemory() const
    {return errorFlags & outOfVideoMemoryFlag ? true : false;}
  bool hasInvalidFrameBufferOperation() const
    {return errorFlags & invalidFrameBufferOperationFlag ? true : false;}

  // other flags check
  bool hasUnkownErrorCode() const
    {return unknownErrorCode != GL_NO_ERROR;}
  GLenum getUnknownErrorCode() const
    {return unknownErrorCode;}
  void setUnknownErrorCode(GLenum unknownErrorCode)
    {this->unknownErrorCode = unknownErrorCode;}

private:
  int errorFlags;
  GLenum unknownErrorCode;
};

// NB: default constructor set flags to succeed
const GLErrorFlags GLErrorFlags::succeed;

//////////////////////////////////////////////////////////////////////////////

class GLContext; // predeclaration
template <typename GLPropertyType>
class GLProperty
{
public:
  typedef GLPropertyType GLType;

  GLProperty(GLContext& context)
    : context(context) {}
  virtual ~GLProperty() {}

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLType& value) const = 0;

  // methods
  virtual GLType getDefaultValue() const
    {return GLType(0);}

  bool isDefaultValue() const // Warning: can produce OpenGL flushing performance penalty
  {
    GLType value;
    if (getValue(value).hasErrors())
      return false;
    return getDefaultValue() == value;
  }

  GLContext& getContext() const
    {return context;}

# ifdef GLEW_MX
  GLEWContext* glewGetContext() const
    {return context.glewGetContext();}
# endif // GLEW_MX

protected:
  GLContext& context;
};

//////////////////////////////////////////////////////////////////////////////

template <typename GLRegisterType>
class GLRegister : public GLProperty<GLRegisterType>
{
public:
  typedef GLRegisterType GLType;
   // construction
  GLRegister(GLContext& context)
    : GLProperty<GLRegisterType>(context) {}

  // interface
  virtual GLErrorFlags setValue(const GLType& value) = 0;
};

//////////////////////////////////////////////////////////////////////////////

template <typename GLCachedRegisterType>
class GLCachedRegister : public GLRegister<GLCachedRegisterType>
{
  typedef GLCachedRegisterType GLType;
  typedef GLRegister<GLType> GLRegisterType;

public:
  GLCachedRegister(GLRegisterType& decorated)
    : GLRegister<GLType>(decorated.getContext())
    , decorated(decorated)
    , currentValue(decorated.getDefaultValue())
    {}

  virtual GLErrorFlags getValue(GLType& value) const
  {
    value = currentValue;
    return GLErrorFlags::succeed;
  }

  // interface
  virtual GLErrorFlags setValue(const GLType& value)
  {
    if (value == currentValue)
      return GLErrorFlags::succeed;
    const GLErrorFlags errorFlags = decorated.setValue(value);
    if (errorFlags.hasSucceed())
      currentValue = value;
    return errorFlags;
  }

  // last chance methods: Warning: can produce OpenGL flushing performance penalty
  // if return false, ensure GLCachedRegister construction have been done just after OpenGL context creation.
  bool isConsistent() const
  {
    GLType value;
    if (decorated.getValue(value).hasErrors())
      return false;
    return value == currentValue;
  }
  GLErrorFlags makeConsistent() const
    {return decorated.getValue(currentValue);}

protected:
  GLRegisterType& decorated;
  GLType currentValue;
};

//////////////////////////////////////////////////////////////////////////////

struct GLColor
{
  enum {
    redId   = 0,
    greenId = 1,
    blueId  = 2,
    alphaId = 3,

    numComponents = 4
  };

  GLColor(GLfloat red = 0.0f, GLfloat green = 0.0f, GLfloat blue = 0.0f, GLfloat alpha = 1.0f)
  {
    components[redId] = red;
    components[greenId] = green;
    components[blueId] = blue;
    components[alphaId] = alpha;
  }

  GLColor(GLint red, GLint green = 0, GLint blue = 0, GLint alpha = 255)
  {
    components[redId] = static_cast<float>(red) / 255.f;
    components[greenId] = static_cast<float>(green) / 255.f;
    components[blueId] = static_cast<float>(blue) / 255.f;
    components[alphaId] = static_cast<float>(alpha) / 255.f;
  }

  GLColor(const GLfloat* components)
    {memcpy(this->components, components, sizeof(GLfloat) * numComponents);}

  GLColor(const GLColor& other)
    {memcpy(components, other.components, sizeof(GLfloat) * numComponents);}

  bool operator ==(const GLColor& other) const
    {return !memcmp(components, other.components, sizeof(GLfloat) * numComponents);}

  const GLfloat& getRed() const
    {return components[redId];}
  const GLfloat& getGreen() const
    {return components[greenId];}
  const GLfloat& getBlue() const
    {return components[blueId];}
  const GLfloat& getAlpha() const
    {return components[alphaId];}

  GLfloat& getRed()
    {return components[redId];}
  GLfloat& getGreen()
    {return components[greenId];}
  GLfloat& getBlue()
    {return components[blueId];}
  GLfloat& getAlpha()
    {return components[alphaId];}

  GLfloat components[numComponents];
};

//////////////////////////////////////////////////////////////////////////////

struct GLRegion
{
  enum {
    leftId    = 0,
    bottomId  = 1,
    widthId   = 2,
    heightId  = 3,

    numDimensions = 4
  };

  GLRegion()
  {
    dimensions[leftId] = 0;
    dimensions[bottomId] = 0;
    dimensions[widthId] = 0; // Warning: default value must be the current OpenGL window client size.
    dimensions[heightId] = 0; // Warning: default value must be the current OpenGL window client size.
  }

  GLRegion(GLint width, GLint height)
  {
    dimensions[leftId] = 0;
    dimensions[bottomId] = 0;
    dimensions[widthId] = width;
    dimensions[heightId] = height;
  }

  GLRegion(GLint left, GLint bottom, GLint width, GLint height)
  {
    dimensions[leftId] = left;
    dimensions[bottomId] = bottom;
    dimensions[widthId] = width;
    dimensions[heightId] = height;
  }

  GLRegion(const GLint* dimensions)
    {memcpy(this->dimensions, dimensions, sizeof(GLint) * numDimensions);}

  GLRegion(const GLRegion& other)
    {memcpy(dimensions, other.dimensions, sizeof(GLint) * numDimensions);}

  bool operator ==(const GLRegion& other) const
    {return !memcmp(dimensions, other.dimensions, sizeof(GLint) * numDimensions);}

  const GLint& getLeft() const
    {return dimensions[leftId];}
  const GLint& getBottom() const
    {return dimensions[bottomId];}
  const GLint& getWidth() const
    {return dimensions[widthId];}
  const GLint& getHeight() const
    {return dimensions[heightId];}

  GLint& getLeft()
    {return dimensions[leftId];}
  GLint& getBottom()
    {return dimensions[bottomId];}
  GLint& getWidth()
    {return dimensions[widthId];}
  GLint& getHeight()
    {return dimensions[heightId];}

  GLint dimensions[numDimensions];
};

//////////////////////////////////////////////////////////////////////////////

struct GLPixelStore
{
  GLPixelStore(GLint rowByteAligment = 4,
                GLint optionalPaddedImageWidth = 0,                           // for 2D textures
                GLint optionalPaddedImageHeight = 0,                          // for 3D textures
                GLboolean haveLittleEndianComponentBytesOrdering = GL_FALSE,  // WARNING: internal component bytes order, not component order.
                GLboolean haveReversedBitsOrdering = GL_FALSE,
                GLint numSkippedPixels = 0,
                GLint numSkippedRows = 0,                                     // for 2D textures
                GLint numSkippedImages = 0)                                   // for 3D textures
    : rowByteAligment(rowByteAligment)
    , optionalPaddedImageWidth(optionalPaddedImageWidth)
    , optionalPaddedImageHeight(optionalPaddedImageHeight)
    , haveLittleEndianComponentBytesOrdering(haveLittleEndianComponentBytesOrdering)
    , haveReversedBitsOrdering(haveReversedBitsOrdering)
    , numSkippedPixels(numSkippedPixels)
    , numSkippedRows(numSkippedRows)
    , numSkippedImages(numSkippedImages)
    {jassert(isValid());}

  static bool isValidRowByteAligment(GLint rowAligment)
    {return (rowAligment == 1) || (rowAligment == 2) || (rowAligment == 4) || (rowAligment == 8);}

  bool isValid() const
  {
    return isValidRowByteAligment(rowByteAligment)
      && optionalPaddedImageWidth >= 0
      && optionalPaddedImageHeight >= 0
      && numSkippedPixels >= 0
      && numSkippedRows >= 0
      && numSkippedImages >= 0;
  }

  bool operator ==(const GLPixelStore& other) const
  {
    return rowByteAligment == other.rowByteAligment
      && optionalPaddedImageWidth == other.optionalPaddedImageWidth
      && optionalPaddedImageHeight == other.optionalPaddedImageHeight
      && haveLittleEndianComponentBytesOrdering == other.haveLittleEndianComponentBytesOrdering
      && haveReversedBitsOrdering == other.haveReversedBitsOrdering
      && numSkippedPixels == other.numSkippedPixels
      && numSkippedRows == other.numSkippedRows
      && numSkippedImages == other.numSkippedImages;
  }

  GLint rowByteAligment;
  GLint optionalPaddedImageWidth;
  GLint optionalPaddedImageHeight;
  GLboolean haveLittleEndianComponentBytesOrdering;
  GLboolean haveReversedBitsOrdering;
  GLint numSkippedPixels;
  GLint numSkippedRows;
  GLint numSkippedImages;
};

//////////////////////////////////////////////////////////////////////////////

struct GLPolygonOffset
{
  GLPolygonOffset(GLfloat factor = 0.0f, GLfloat units = 0.0f)
    : factor(factor), units(units) {}

  GLfloat  	factor;
  GLfloat  	units;
};

//////////////////////////////////////////////////////////////////////////////

class GLContext
{
public:
  // construction
  virtual GLErrorFlags initialize()  = 0;
  virtual bool isInitialized() const = 0;

  // environment
  virtual const char* getVendorName() const = 0;
  virtual const char* getRenderName() const = 0;
  virtual const char* getVersion() const = 0;
  virtual const char* getShadingLanguageVersion() const = 0;

  // hints
  virtual GLRegister<GLenum>& getLineSmoothHint() = 0;
  virtual GLRegister<GLenum>& getPolygonSmoothHint() = 0;
  virtual GLRegister<GLenum>& getTextureCompressionQualityHint() = 0;
  virtual GLRegister<GLenum>& getFragmentShaderDerivativeAccuracyHint() = 0;

  // error flags
  // WARNING: When it's possible, prefer jassertglsucceed(context) to discard potential OpenGL state flushing penalty in release build.
  virtual GLErrorFlags popErrorFlags() = 0;
  virtual void clearErrorFlags() = 0;

  // clear
  virtual GLRegister<GLColor>& getClearColor() = 0;
  virtual GLRegister<GLfloat>& getClearDepth() = 0;
  virtual GLRegister<GLint>& getClearStencil() = 0;
  virtual GLErrorFlags clear(GLbitfield mask) = 0;

  // viewport
  virtual GLRegister<GLRegion>& getActiveViewport() = 0;
  virtual GLint getMaxViewportWidth() const = 0;
  virtual GLint getMaxViewportHeight() const = 0;

  // scissor
  virtual GLRegister<GLRegion>& getActiveScissor() = 0;
  virtual GLRegister<GLboolean>& getScissorTest() = 0;

  // depth
  virtual GLRegister<GLboolean>& getDepthTest() = 0;

  // points
  virtual GLfloat getPointSmallestSize() const = 0;
  virtual GLfloat getPointLargestSize() const = 0;
  virtual GLfloat getPointSizeGranularity() const = 0;
  virtual GLRegister<GLboolean>& getPointSizeProgrammable() = 0;
  virtual GLRegister<GLfloat>& getPointSize() = 0;
  virtual GLRegister<GLboolean>& getPointPolygonOffset() = 0;

  // line
  virtual GLRegister<GLboolean>& getLinePolygonOffset() = 0;

  // polygon
  virtual GLRegister<GLboolean>& getCullFace() = 0;
  virtual GLRegister<GLboolean>& getFillPolygonOffset() = 0;
  virtual GLRegister<GLPolygonOffset>& getPolygonOffset() = 0;

  // multi texturing
  virtual GLenum getNumTextureUnits() const = 0;
  virtual bool isValidTextureUnitIndex(GLenum index) const = 0;
  virtual GLRegister<GLenum>& getActiveTextureUnit() = 0;

  // texturing
  virtual size_t getMaxTextureSize() const = 0;
  virtual bool isValidTextureSize(GLsizei size) const = 0;
  virtual bool isValidTextureBindingTarget(GLenum target) const = 0;
  virtual GLRegister<GLuint>& getActiveTextureBind(GLenum target) = 0;

  // pixel store: pack=VRAM->RAM, unpack=RAM->VRAM
  virtual GLRegister<GLint>&        getPixelStoreRowByteAligment(bool packing) = 0;
  virtual GLRegister<GLint>&        getPixelStorePaddedImageWidth(bool packing) = 0;
  virtual GLRegister<GLint>&        getPixelStorePaddedImageHeight(bool packing) = 0;
  virtual GLRegister<GLboolean>&    getPixelStoreHaveLittleEndianComponentBytesOrdering(bool packing) = 0;
  virtual GLRegister<GLboolean>&    getPixelStoreHaveReversedBitsOrdering(bool packing) = 0;
  virtual GLRegister<GLint>&        getPixelStoreNumSkippedPixels(bool packing) = 0;
  virtual GLRegister<GLint>&        getPixelStoreNumSkippedRows(bool packing) = 0;
  virtual GLRegister<GLint>&        getPixelStoreNumSkippedImages(bool packing) = 0;
  virtual GLRegister<GLPixelStore>& getPixelStore(bool packing) = 0;

  // buffer
  virtual bool isValidBufferBindingTarget(GLenum target) const = 0;
  virtual GLRegister<GLuint>& getActiveBufferBind(GLenum target) = 0;

  // vertex array.
  virtual bool isValidVertexAttributeIndex(GLuint index) = 0;
  virtual GLuint getNumVertexAttributes() const = 0;
  virtual GLRegister<GLuint>& getActiveVertexArrayBind() = 0;

  // program
  virtual bool isValidShaderType(GLenum shaderType) const = 0;
  virtual GLRegister<GLenum>& getActiveProgramBind() = 0;

  // Opengl 4.1 subroutines
  virtual GLErrorFlags loadUniformSubroutines(GLenum shaderType, GLsizei count, const GLuint* indices) = 0; // need to be called after each getActiveProgramBind().setValue
  virtual GLuint getMaxSubRoutines() const = 0;
  virtual GLuint getMaxSubRoutinesUniformLocation() const = 0;

  // program pipeline
  virtual GLRegister<GLenum>& getActiveProgramPipelineBind() = 0;

  // multi context glew abstraction
# ifdef GLEW_MX
  virtual GLEWContext* glewGetContext() const = 0;
# endif // GLEW_MX
};

# if defined(DEBUG) || defined(_DEBUG)
#  define jassertglsucceed(context) {const GLErrorFlags errorFlags = context.popErrorFlags(); jassert(errorFlags.hasSucceed());}
# else
#  define jassertglsucceed(context)
# endif // ! defined(DEBUG) || defined(_DEBUG)


//////////////////////////////////////////////////////////////////////////////

class GLIntegerConstantsStore
{
public:
  GLIntegerConstantsStore(GLContext& context)
    : context(context) {}

  // return succeed else check getFailureId to know witch id is unknown
  GLErrorFlags set(const GLenum* constantIDs, size_t numConstants)
  {
    context.clearErrorFlags();
    for (size_t i = 0; i < numConstants; ++i)
    {
      const GLenum id = constantIDs[i];
      GLErrorFlags errorFlags;
      if (id == GL_MAX_VIEWPORT_DIMS)
        errorFlags = getMaxViewportDims(); // special case for two dimensional int
      else
      {
        GLint value;
        glGetIntegerv(id, &value);
        GLErrorFlags errorFlags = context.popErrorFlags();
        if (errorFlags.hasSucceed())
          constants[id] = value;
      }
      if (errorFlags.hasErrors())
      {
        failureId = id;
        constants.clear();
        return errorFlags;
      }
    }
    return GLErrorFlags::succeed;
  }

  // valid only if set method has failed
  GLenum getFailureId()
    {return failureId;}

  bool isConstantStored(GLenum id) const
    {return constants.find(id) != constants.end();}

  GLint getValue(GLenum id) const
  {
    ConstantsStore::const_iterator it = constants.find(id);
    if (it == constants.end())
    {
      jassertfalse; // unknown id: check the set parameters and return value.
      return 0;
    }
    return it->second;
  }

  // internal DOCGL property, hope than never collide with other gl constants
  enum {GL_MAX_VIEWPORT_WIDTH = 0xBABA0000 | GL_MAX_VIEWPORT_DIMS};
  enum {GL_MAX_VIEWPORT_HEIGHT = 0xDEAD0000 | GL_MAX_VIEWPORT_DIMS};

private:
  GLErrorFlags getMaxViewportDims()
  {
    GLint values[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, values);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
      return errorFlags;
    constants[GL_MAX_VIEWPORT_WIDTH] = values[0];
    constants[GL_MAX_VIEWPORT_HEIGHT] = values[1];
    return GLErrorFlags::succeed;
  }

private:
  GLContext& context;
  typedef std::map<GLenum, GLint> ConstantsStore; // [id] = value
  ConstantsStore constants;
  GLenum failureId;
};

//////////////////////////////////////////////////////////////////////////////

class GLStringConstantsStore
{
public:
  GLStringConstantsStore(GLContext& context)
    : context(context) {}

  // return succeed else check getFailureId to know witch id is unknown
  GLErrorFlags set(const GLenum* constantIDs, size_t numConstants)
  {
    context.clearErrorFlags();
    for (size_t i = 0; i < numConstants; ++i)
    {
      const GLenum id = constantIDs[i];
      const char* value = reinterpret_cast<const char*>(glGetString(id));
      const GLErrorFlags errorFlags = context.popErrorFlags();
      if (errorFlags.hasErrors())
      {
        failureId = id;
        constants.clear();
        return errorFlags;
      }
      constants[id] = value;
    }
    return GLErrorFlags::succeed;
  }

  // valid only if set method has failed
  GLenum getFailureId()
    {return failureId;}

  bool isConstantStored(GLenum id) const
    {return constants.find(id) != constants.end();}

  const char* getValue(GLenum id) const
  {
    ConstantsStore::const_iterator it = constants.find(id);
    if (it == constants.end())
    {
      jassertfalse; // unknown id: check the set parameters and return value.
      return 0;
    }
    return it->second;
  }

private:
  GLContext& context;
  typedef std::map<GLenum, const char*> ConstantsStore; // [id] = value
  ConstantsStore constants;
  GLenum failureId;
};

//////////////////////////////////////////////////////////////////////////////

class GLFloatConstantsStore
{
public:
  GLFloatConstantsStore(GLContext& context)
    : context(context) {}

  // return succeed else check getFailureId to know witch id is unknown
  GLErrorFlags set(const GLenum* constantIDs, size_t numConstants)
  {
    context.clearErrorFlags();
    for (size_t i = 0; i < numConstants; ++i)
    {
      const GLenum id = constantIDs[i];
      GLErrorFlags errorFlags;
      if (id == GL_POINT_SIZE_RANGE)
         errorFlags = getPointSizeRange(); // special case for two dimensional float
      else
      {
        GLfloat value;
        glGetFloatv(id, &value);
        errorFlags = context.popErrorFlags();
        if (errorFlags.hasSucceed())
          constants[id] = value;
      }
      if (errorFlags.hasErrors())
      {
        failureId = id;
        constants.clear();
        return errorFlags;
      }
    }
    return GLErrorFlags::succeed;
  }

  // valid only if set method has failed
  GLenum getFailureId()
    {return failureId;}

  bool isConstantStored(GLenum id) const
    {return constants.find(id) != constants.end();}

  GLfloat getValue(GLenum id) const
  {
    ConstantsStore::const_iterator it = constants.find(id);
    if (it == constants.end())
    {
      jassertfalse; // unknown id: check the set parameters and return value.
      return 0;
    }
    return it->second;
  }

  // internal DOCGL property, hope than never collide with other gl constants
  enum {GL_POINT_SMALLEST_SIZE = 0xBABA0000 | GL_POINT_SIZE_RANGE};
  enum {GL_POINT_LARGEST_SIZE  = 0xDEAD0000 | GL_POINT_SIZE_RANGE};

private:
  GLErrorFlags getPointSizeRange()
  {
    GLfloat values[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, values);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
      return errorFlags;
    constants[GL_POINT_SMALLEST_SIZE] = values[0];
    constants[GL_POINT_LARGEST_SIZE] = values[1];
    return GLErrorFlags::succeed;
  }
private:
  GLContext& context;
  typedef std::map<GLenum, GLfloat> ConstantsStore; // [id] = value
  ConstantsStore constants;
  GLenum failureId;
};

//////////////////////////////////////////////////////////////////////////////

template <GLenum capability, GLboolean defaultValue = GL_FALSE>
class GLBooleanRegister : public GLRegister<GLboolean>
{
public:
  GLBooleanRegister(GLContext& context)
    : GLRegister<GLboolean>(context) {}

  virtual GLboolean getDefaultValue() const
    {return defaultValue;}

  virtual GLErrorFlags setValue(const GLboolean& enabled)
  {
    if (enabled == GL_FALSE)
      glDisable(capability);
    else
      glEnable(capability);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLboolean& id) const
  {
    context.clearErrorFlags();
    id = glIsEnabled(capability);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

template <GLenum target>
class GLHint : public GLRegister<GLenum>
{
public:
  GLHint(GLContext& context)
    : GLRegister<GLenum>(context) {}

  virtual GLenum getDefaultValue() const
    {return GL_DONT_CARE;}

  virtual GLErrorFlags setValue(const GLenum& mode)
  {
    if (!isValidHintMode(mode))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    glHint(target, mode);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLenum& id) const
  {
    GLint intValue = GL_DONT_CARE;
    context.clearErrorFlags();
    glGetIntegerv(target, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if((intValue < 0) || !isValidHintMode(intValue))
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    id = static_cast<GLenum>(intValue);
    return errorFlags;
  }

  static bool isValidHintMode(GLenum mode)
    {return mode == GL_DONT_CARE || mode == GL_FASTEST || mode == GL_NICEST;}
};

//////////////////////////////////////////////////////////////////////////////

class GLClearColor : public GLRegister<GLColor>
{
public:
  GLClearColor(GLContext& context)
    : GLRegister<GLColor>(context) {}

  virtual GLErrorFlags setValue(const GLColor& color)
  {
    glClearColor(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLColor& color) const
  {
    context.clearErrorFlags();
    glGetFloatv(GL_COLOR_CLEAR_VALUE, color.components);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  virtual GLColor getDefaultValue() const
    {return GLColor(0.0f, 0.0f, 0.0f, 0.0f);}
};

//////////////////////////////////////////////////////////////////////////////

class GLActivePolygonOffset : public GLRegister<GLPolygonOffset>
{
public:
  GLActivePolygonOffset(GLContext& context)
    : GLRegister<GLPolygonOffset>(context) {}

  virtual GLErrorFlags setValue(const GLPolygonOffset& polygonOffset)
  {
    glPolygonOffset(polygonOffset.factor, polygonOffset.units);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLPolygonOffset& polygonOffset) const
  {
    context.clearErrorFlags();
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &polygonOffset.factor);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (!errorFlags.hasSucceed())
    {
      jassertfalse;
      return errorFlags;
    }
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &polygonOffset.units);
    errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

class GLClearDepth : public GLRegister<GLfloat>
{
public:
  GLClearDepth(GLContext& context)
    : GLRegister<GLfloat>(context) {}

  virtual GLErrorFlags setValue(const GLfloat& value)
  {
    jassert(value >= 0.0f && value <= 1.0f); // clamp needed
    glClearDepth(value);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLfloat& value) const
  {
    context.clearErrorFlags();
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &value);
    GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  virtual GLfloat getDefaultValue() const
    {return 1.0f;}
};

//////////////////////////////////////////////////////////////////////////////

class GLClearStencil : public GLRegister<GLint>
{
public:
  GLClearStencil(GLContext& context)
    : GLRegister<GLint>(context) {}

  virtual GLErrorFlags setValue(const GLint& value)
  {
    glClearStencil(value);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLint& value) const
  {
    context.clearErrorFlags();
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &value);
    GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

class GLActiveViewport : public GLRegister<GLRegion>
{
public:
  GLActiveViewport(GLContext& context)
    : GLRegister<GLRegion>(context) {}

  virtual GLErrorFlags setValue(const GLRegion& region)
  {
    if (region.getWidth() < 0 || region.getHeight() < 0)
      return GLErrorFlags::invalidValueFlag;
    jassert(region.getWidth() <= context.getMaxViewportWidth());
    jassert(region.getHeight() <= context.getMaxViewportHeight());
    glViewport(region.getLeft(), region.getBottom(), region.getWidth(), region.getHeight());
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLRegion& region) const
  {
    context.clearErrorFlags();
    glGetIntegerv(GL_VIEWPORT, region.dimensions);
    GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    jassert(errorFlags.hasErrors() || region.getWidth() <= context.getMaxViewportWidth());
    jassert(errorFlags.hasErrors() || region.getHeight() <= context.getMaxViewportHeight());
    return errorFlags;
  }

  // warning: Default value is not correct and must be set.
};


//////////////////////////////////////////////////////////////////////////////

class GLActiveScissor : public GLRegister<GLRegion>
{
public:
  GLActiveScissor(GLContext& context)
    : GLRegister<GLRegion>(context) {}

  virtual GLErrorFlags setValue(const GLRegion& region)
  {
    if (region.getWidth() < 0 || region.getHeight() < 0)
      return GLErrorFlags::invalidValueFlag;
    glScissor(region.getLeft(), region.getBottom(), region.getWidth(), region.getHeight());
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLRegion& region) const
  {
    context.clearErrorFlags();
    glGetIntegerv(GL_SCISSOR_BOX, region.dimensions);
    GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  // warning: Default value is not correct and must be set.
};

//////////////////////////////////////////////////////////////////////////////

class GLPointSize : public GLRegister<GLfloat>
{
public:
  GLPointSize(GLContext& context)
    : GLRegister<GLfloat>(context) {}

  virtual GLfloat getDefaultValue() const
    {return 1.0f;}

  virtual GLErrorFlags setValue(const GLfloat& size)
  {
    // TODO ? check if size is in the implementation range
    glPointSize(size);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLfloat& size) const
  {
    size = 1.0f;
    context.clearErrorFlags();
    glGetFloatv(GL_POINT_SIZE, &size);
    GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    // TODO ? check if size is in the implementation range
    return errorFlags;
  }

  static bool isValidHintMode(GLenum mode)
    {return mode == GL_DONT_CARE || mode == GL_FASTEST || mode == GL_NICEST;}
};

//////////////////////////////////////////////////////////////////////////////

class GLActiveTextureUnit : public GLRegister<GLenum>
{
public:
  GLActiveTextureUnit(GLContext& context)
    : GLRegister<GLenum>(context) {}

  // index can be GL_TEXTURE0 GL_TEXTURE1 GL_TEXTURE2...
  virtual GLErrorFlags setValue(const GLenum& index)
  {
    if (!context.isValidTextureUnitIndex(index))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    glActiveTexture(index);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // return texture unit ID based on GL_TEXTURE0
  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLenum& id) const
  {
    GLint intValue = GL_TEXTURE0;
    context.clearErrorFlags();
    glGetIntegerv(GL_ACTIVE_TEXTURE, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if((intValue < 0) || !context.isValidTextureUnitIndex(intValue))
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    id = static_cast<GLenum>(intValue);
    return errorFlags;
  }

  virtual GLenum getDefaultValue() const
    {return GL_TEXTURE0;}
};

//////////////////////////////////////////////////////////////////////////////

template<GLenum target, GLenum targetBinding>
class GLActiveTextureBind : public GLRegister<GLuint>
{
public:
  GLActiveTextureBind(GLContext& context)
    : GLRegister<GLuint>(context) {}

  // textureID must be generated by glGenTextures
  // WARNING1: Texure first bound define the target kind of a new generated texture.
  // WARNING2: glIsTexture on a textureID is true only after a first bound.
  virtual GLErrorFlags setValue(const GLuint& textureId)
  {
    if (!context.isValidTextureBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    glBindTexture(target, textureId);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLuint& textureId) const
  {
    GLint intValue = 0;
    context.clearErrorFlags();
    glGetIntegerv(targetBinding, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if((intValue < 0))
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    textureId = static_cast<GLuint>(intValue);
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

template <class GLPixelStoreType, GLenum name, GLPixelStoreType defaultValue = 0>
class GLPixelStoreSubRegister : public GLRegister<GLPixelStoreType>
{
public:
  typedef GLPixelStoreType GLType;
  GLPixelStoreSubRegister(GLContext& context)
  : GLRegister<GLType>(context) {}

  virtual GLErrorFlags setValue(const GLType& param)
  {
    if (!isValidName(name))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    if (!isValidParameter(name, param))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    glPixelStorei(name, param);
    jassertglsucceed(GLRegister<GLType>::context);
    return GLErrorFlags::succeed;
  }

  virtual GLType getDefaultValue() const
    {return defaultValue;}

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLType& param) const
  {
    GLint intValue = defaultValue;
    GLRegister<GLType>::context.clearErrorFlags();
    glGetIntegerv(name, &intValue);
    GLErrorFlags errorFlags = GLRegister<GLType>::context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if ((intValue < 0) || !isValidParameter(name, intValue))
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

  static bool isValidRowByteAligment(GLType rowAligment)
    {return (rowAligment == 1) || (rowAligment == 2) || (rowAligment == 4) || (rowAligment == 8);}

  static bool isValidParameter(GLenum parameterName, GLType parameter)
  {
    if (parameterName== GL_PACK_ALIGNMENT || parameterName == GL_UNPACK_ALIGNMENT)
      return isValidRowByteAligment(parameter);
    else
      return parameter >= 0;
  }

  static bool isValidName(GLenum storeName)
  {
    switch (storeName)
    {
    case GL_PACK_ALIGNMENT:
    case GL_PACK_ROW_LENGTH:
    case GL_PACK_IMAGE_HEIGHT:
    case GL_PACK_SWAP_BYTES:
    case GL_PACK_LSB_FIRST:
    case GL_PACK_SKIP_PIXELS:
    case GL_PACK_SKIP_ROWS:
    case GL_PACK_SKIP_IMAGES:
    case GL_UNPACK_ALIGNMENT:
    case GL_UNPACK_ROW_LENGTH:
    case GL_UNPACK_IMAGE_HEIGHT:
    case GL_UNPACK_SWAP_BYTES:
    case GL_UNPACK_LSB_FIRST:
    case GL_UNPACK_SKIP_PIXELS:
    case GL_UNPACK_SKIP_ROWS:
    case GL_UNPACK_SKIP_IMAGES:
      return true;
    default:
      return false;
    }
  }
};

//////////////////////////////////////////////////////////////////////////////

template <bool packing>
class GLPixelStoreRegister : public GLRegister<GLPixelStore>
{
public:
  // construction
  GLPixelStoreRegister(GLContext& context)  // pack: VRAM->RAM, unpack: RAM->VRAM
    : GLRegister<GLPixelStore>(context) {}

  // Warning: can produce OpenGL flushing performance penalty
  virtual GLErrorFlags getValue(GLPixelStore& pixelStore) const
  {
    GLErrorFlags errorFlags = context.getPixelStoreRowByteAligment(packing).getValue(pixelStore.rowByteAligment);
    errorFlags.merge(context.getPixelStorePaddedImageWidth(packing).getValue(pixelStore.optionalPaddedImageWidth));
    errorFlags.merge(context.getPixelStorePaddedImageHeight(packing).getValue(pixelStore.optionalPaddedImageHeight));
    errorFlags.merge(context.getPixelStoreHaveLittleEndianComponentBytesOrdering(packing).getValue(pixelStore.haveLittleEndianComponentBytesOrdering));
    errorFlags.merge(context.getPixelStoreHaveReversedBitsOrdering(packing).getValue(pixelStore.haveReversedBitsOrdering));
    errorFlags.merge(context.getPixelStoreNumSkippedPixels(packing).getValue(pixelStore.numSkippedPixels));
    errorFlags.merge(context.getPixelStoreNumSkippedRows(packing).getValue(pixelStore.numSkippedRows));
    errorFlags.merge(context.getPixelStoreNumSkippedImages(packing).getValue(pixelStore.numSkippedImages));
    return errorFlags;
  }

  virtual GLPixelStore getDefaultValue() const
    {return GLPixelStore();} // assume that GLPixelStore default constructor set OpenGL default values

  virtual GLErrorFlags setValue(const GLPixelStore& pixelStore)
  {
    if (!pixelStore.isValid())
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    GLErrorFlags errorFlags;
    errorFlags.merge(context.getPixelStoreRowByteAligment(packing).setValue(pixelStore.rowByteAligment));
    errorFlags.merge(context.getPixelStorePaddedImageWidth(packing).setValue(pixelStore.optionalPaddedImageWidth));
    errorFlags.merge(context.getPixelStorePaddedImageHeight(packing).setValue(pixelStore.optionalPaddedImageHeight));
    errorFlags.merge(context.getPixelStoreHaveLittleEndianComponentBytesOrdering(packing).setValue(pixelStore.haveLittleEndianComponentBytesOrdering));
    errorFlags.merge(context.getPixelStoreHaveReversedBitsOrdering(packing).setValue(pixelStore.haveReversedBitsOrdering));
    errorFlags.merge(context.getPixelStoreNumSkippedPixels(packing).setValue(pixelStore.numSkippedPixels));
    errorFlags.merge(context.getPixelStoreNumSkippedRows(packing).setValue(pixelStore.numSkippedRows));
    errorFlags.merge(context.getPixelStoreNumSkippedImages(packing).setValue(pixelStore.numSkippedImages));
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

template<GLenum target, GLenum targetBinding = 0 /* for read only buffer bind register */>
class GLActiveBufferBind : public GLRegister<GLuint>
{
public:
  GLActiveBufferBind(GLContext& context)
    : GLRegister<GLuint>(context) {}

  // textureID must be generated by glGenVertexArrays
  // WARNING2: glIsTexture on a textureID is true only after a first bound.
  virtual GLErrorFlags setValue(const GLuint& bufferID)
  {
    if (!context.isValidBufferBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    glBindBuffer(target, bufferID);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& bufferId) const
  {
    GLint intValue = 0;
    jassert(targetBinding != GL_TEXTURE_BINDING_BUFFER); // not tested yet
    if (targetBinding == 0)
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;} // could not get current value for GL_COPY_READ_BUFFER / GL_COPY_WRITE_BUFFER (write only register)
    context.clearErrorFlags();
    glGetIntegerv(targetBinding, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    bufferId = static_cast<GLType>(intValue);
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

class GLActiveVertexArrayBind : public GLRegister<GLuint>
{
public:
  GLActiveVertexArrayBind(GLContext& context)
    : GLRegister<GLuint>(context) {}

  // vertexArrayID must be generated by glGenVertexArrays
  virtual GLErrorFlags setValue(const GLuint& vertexArrayId)
  {
    glBindVertexArray(vertexArrayId);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLuint& vertexArrayId) const
  {
    GLint intValue = 0;
    context.clearErrorFlags();
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    vertexArrayId = static_cast<GLuint>(intValue);
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

class GLActiveProgramBind : public GLRegister<GLuint>
{
public:
  GLActiveProgramBind(GLContext& context)
    : GLRegister<GLuint>(context) {}

  // programId must be created by glCreateProgram
  virtual GLErrorFlags setValue(const GLuint& programId)
  {
    glUseProgram(programId);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // return the current program id
  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLuint& programId) const
  {
    GLint intValue = 0;
    context.clearErrorFlags();
    glGetIntegerv(GL_CURRENT_PROGRAM, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    programId = static_cast<GLuint>(intValue);
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

class GLActiveProgramPipelineBind : public GLRegister<GLuint>
{
public:
  GLActiveProgramPipelineBind(GLContext& context)
    : GLRegister<GLuint>(context) {}

  // programId must be created by glCreateProgram
  virtual GLErrorFlags setValue(const GLuint& programId)
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}  // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    glBindProgramPipeline(programId);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // return the current program id
  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLuint& programId) const
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}  // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    GLint intValue = 0;
    context.clearErrorFlags();
    glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    programId = static_cast<GLuint>(intValue);
    return errorFlags;
  }
};

//////////////////////////////////////////////////////////////////////////////

// Concret GLContext with direct (non cached) OpenGL context access.
class GLDirectContext : public GLContext
{
public:

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4355 ) /** 'this' : used in base member initializer list */
#endif // ! _MSC_VER

  GLDirectContext()
    : initialized(false), integerConstantsStore(*this), stringConstantsStore(*this), floatConstantsStore(*this)

    , lineSmoothHint(*this), polygonSmoothHint(*this)
    , textureCompressionQualityHint(*this), fragmentShaderDerivativeAccuracyHint(*this)

    , clearColor(*this), clearDepth(*this), clearStencil(*this)
    , activeViewport(*this), activeScissor(*this), scissorTest(*this)

    , depthTest(*this)

    , pointSizeProgrammable(*this), pointSize(*this), pointPolygonOffset(*this)

    , linePolygonOffset(*this)

    , cullFace(*this)
    , fillPolygonOffset(*this)
    , polygonOffset(*this)

    , activeTextureUnit(*this), activeTexture1d(*this), activeTexture2d(*this)
    , activeTexture3d(*this), activeTexture1dArray(*this), activeTexture2dArray(*this)
    , activeTextureRectangle(*this), activeTextureBuffer(*this), activeTextureCubeMap(*this)
    , activeTexture2dMultisample(*this), activeTexture2dMultisampleArray(*this)

    , pixelStorePackRowByteAligment(*this), pixelStorePackPaddedImageWidth(*this)
    , pixelStorePackPaddedImageHeight(*this), pixelStorePackHaveLittleEndianComponentBytesOrdering(*this)
    , pixelStorePackHaveReversedBitsOrdering(*this), pixelStorePackNumSkippedPixels(*this)
    , pixelStorePackNumSkippedRows(*this), pixelStorePackNumSkippedImages(*this)
    , pixelStoreUnPackRowByteAligment(*this), pixelStoreUnPackPaddedImageWidth(*this)
    , pixelStoreUnPackPaddedImageHeight(*this), pixelStoreUnPackHaveLittleEndianComponentBytesOrdering(*this)
    , pixelStoreUnPackHaveReversedBitsOrdering(*this), pixelStoreUnPackNumSkippedPixels(*this)
    , pixelStoreUnPackNumSkippedRows(*this), pixelStoreUnPackNumSkippedImages(*this)
    , pixelStorePack(*this), pixelStoreUnPack(*this)

    , activeBufferForArray(*this), activeBufferForCopyRead(*this)
    , activeBufferForCopyWrite(*this), activeBufferForElementArray(*this)
    , activeBufferForPixelPack(*this), activeBufferForPixelUnPack(*this)
    , activeBufferForTexture(*this), activeBufferForTransformFeedback(*this)
    , activeBufferForUniform(*this)

    , activeVertexArrayBind(*this)

    , activeProgramBind(*this)

    , activeProgramPipelineBind(*this)
    {}

#ifdef _MSC_VER
#pragma warning( pop ) // but remember: DO NOT CALL CONTEXT METHODS IN REGISTERS CONSTRUCTORS.
#endif // ! _MSC_VER

  GLErrorFlags initialize()
  {
    clearErrorFlags();
    static const GLenum glIntegerConstants[] = {
      GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,  // for getNumTextureUnits
      GL_MAX_TEXTURE_SIZE,                  // for getMaxTextureSize
      GL_MAX_VERTEX_ATTRIBS,                // for getNumVertexAttributes
      GL_MAX_VIEWPORT_DIMS,                 // for GL_MAX_VIEWPORT_WIDTH GL_MAX_VIEWPORT_HEIGHT -> get
      };
    GLErrorFlags errorFlags = integerConstantsStore.set(glIntegerConstants, sizeof(glIntegerConstants) / sizeof(glIntegerConstants[0]));
    if (errorFlags.hasErrors())
      return errorFlags;

    static const GLenum gl4_1_IntegerConstants[] = {
      GL_MAX_SUBROUTINES,
      GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS,
    };
    errorFlags = integerConstantsStore.set(gl4_1_IntegerConstants, sizeof(gl4_1_IntegerConstants) / sizeof(gl4_1_IntegerConstants[0]));
    jassert(errorFlags.hasSucceed()); // context does'nt support openGL 4.1

    static const GLenum glStringConstants[] = {
      GL_VENDOR,                  // for getVendorName
      GL_RENDERER,                // for getRenderName
      GL_VERSION,                 // for getVersion
      GL_SHADING_LANGUAGE_VERSION,// for getShadingLanguageVersion
    };
    errorFlags = stringConstantsStore.set(glStringConstants, sizeof(glStringConstants) / sizeof(glStringConstants[0]));
    if (errorFlags.hasErrors())
      return errorFlags;

    static const GLenum glFloatConstants[] = {
      GL_POINT_SIZE_RANGE,        // for GL_POINT_SMALLEST_SIZE GL_POINT_LARGEST_SIZE -> get
      GL_POINT_SIZE_GRANULARITY,  // for getPointSizeGranularity
    };

    errorFlags = floatConstantsStore.set(glFloatConstants, sizeof(glFloatConstants) / sizeof(glFloatConstants[0]));
    if (errorFlags.hasSucceed())
      initialized = true;
    return errorFlags;
  }

  virtual bool isInitialized() const
    {return initialized;}

  // environment
  virtual const char* getVendorName() const
    {return stringConstantsStore.getValue(GL_VENDOR);}
  virtual const char* getRenderName() const
    {return stringConstantsStore.getValue(GL_RENDERER);}
  virtual const char* getVersion() const
    {return stringConstantsStore.getValue(GL_VERSION);}
  virtual const char* getShadingLanguageVersion() const
    {return stringConstantsStore.getValue(GL_SHADING_LANGUAGE_VERSION);}

  // Get context error flags
  // WARNING: When it's possible, prefer jassertglsucceed(context) to discard potential OpenGL state flushing penalty in release build.
  virtual GLErrorFlags popErrorFlags()
  {
    GLErrorFlags returnValue;
    GLenum error;
    do
    {
      error = glGetError();
      switch (error)
      {
      case GL_NO_ERROR:
        // do nothing
        break;
      case GL_INVALID_ENUM:
        returnValue.merge(GLErrorFlags::invalidEnumFlag);
        break;
      case GL_INVALID_VALUE:
        returnValue.merge(GLErrorFlags::invalidValueFlag);
        break;
      case GL_INVALID_OPERATION:
        returnValue.merge(GLErrorFlags::invalidOperationFlag);
        break;
      case GL_OUT_OF_MEMORY:
        returnValue.merge(GLErrorFlags::outOfVideoMemoryFlag);
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        returnValue.merge(GLErrorFlags::invalidFrameBufferOperationFlag);
        break;
      default:
        jassertfalse;                           // Unknown or deprecated error code; WARNING: The next loop can overrite this value.
        returnValue.setUnknownErrorCode(error); // WARNING: only the last returned unknown error are returned.
        break;                                  // This Must be done to ensure popContextErrorFlags caller that we leave state without error.
      }
    }
    while (error != GL_NO_ERROR);
    return returnValue;
  }

  // Clear context error flags
  // WARNING: potential OpenGL state flushing penalty.
  virtual void clearErrorFlags()
  {
    const GLErrorFlags errorFlags = popErrorFlags();
    jassert(errorFlags.hasSucceed()); // Some errors have been discarded.
  }

  // hints
  virtual GLRegister<GLenum>& getLineSmoothHint()
    {return lineSmoothHint;}
  virtual GLRegister<GLenum>& getPolygonSmoothHint()
    {return polygonSmoothHint;}
  virtual GLRegister<GLenum>& getTextureCompressionQualityHint()
    {return textureCompressionQualityHint;}
  virtual GLRegister<GLenum>& getFragmentShaderDerivativeAccuracyHint()
    {return fragmentShaderDerivativeAccuracyHint;}

  // clear
  virtual GLRegister<GLColor>& getClearColor()
    {return clearColor;}
  virtual GLRegister<GLfloat>& getClearDepth()
    {return clearDepth;}
  virtual GLRegister<GLint>& getClearStencil()
    {return clearStencil;}
  virtual GLErrorFlags clear(GLbitfield mask)
  {
    if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
      return GLErrorFlags::invalidValueFlag;
    clearErrorFlags();
    glClear(mask);
    const GLErrorFlags errorFlags = popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  // viewport
  virtual GLRegister<GLRegion>& getActiveViewport()
    {return activeViewport;}
  virtual GLint getMaxViewportWidth() const
    {return integerConstantsStore.getValue(GLIntegerConstantsStore::GL_MAX_VIEWPORT_WIDTH);}
  virtual GLint getMaxViewportHeight() const
    {return integerConstantsStore.getValue(GLIntegerConstantsStore::GL_MAX_VIEWPORT_HEIGHT);}

  // scissor
  virtual GLRegister<GLRegion>& getActiveScissor()
    {return activeScissor;}
  virtual GLRegister<GLboolean>& getScissorTest()
    {return scissorTest;}

  // depth
  virtual GLRegister<GLboolean>& getDepthTest()
    {return depthTest;}

  // points
  virtual GLfloat getPointSmallestSize() const
    {return floatConstantsStore.getValue(GLFloatConstantsStore::GL_POINT_SMALLEST_SIZE);}
  virtual GLfloat getPointLargestSize() const
    {return floatConstantsStore.getValue(GLFloatConstantsStore::GL_POINT_LARGEST_SIZE);}
  virtual GLfloat getPointSizeGranularity() const
    {return floatConstantsStore.getValue(GL_POINT_SIZE_GRANULARITY);}
  virtual GLRegister<GLboolean>& getPointSizeProgrammable()
    {return pointSizeProgrammable;}
  virtual GLRegister<GLfloat>& getPointSize()
    {return pointSize;}
  virtual GLRegister<GLboolean>& getPointPolygonOffset()
    {return pointPolygonOffset;}

  // line
  virtual GLRegister<GLboolean>& getLinePolygonOffset()
    {return linePolygonOffset;}

  // polygon
  virtual GLRegister<GLboolean>& getCullFace()
    {return cullFace;}
  virtual GLRegister<GLboolean>& getFillPolygonOffset()
    {return fillPolygonOffset;}
  virtual GLRegister<GLPolygonOffset>& getPolygonOffset()
    {return polygonOffset;}

  // multitexture
  virtual GLenum getNumTextureUnits() const
    {return integerConstantsStore.getValue(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);}
  virtual bool isValidTextureUnitIndex(GLenum index) const
    {return (index >= GL_TEXTURE0 && index <= (GL_TEXTURE0 + getNumTextureUnits()));}
  virtual GLRegister<GLenum>& getActiveTextureUnit()
    {return activeTextureUnit;}

  // texture
  virtual size_t getMaxTextureSize() const
    {return integerConstantsStore.getValue(GL_MAX_TEXTURE_SIZE);}
  virtual bool isValidTextureSize(GLsizei size) const
    {return size >= 0 && (size_t)size <= getMaxTextureSize();}
  virtual bool isValidTextureBindingTarget(GLenum target) const
  {
    switch (target)
    {
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_RECTANGLE:
    case GL_TEXTURE_BUFFER:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_2D_MULTISAMPLE:
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return true;
    default:
      return false;
    }
  }
  virtual GLRegister<GLuint>& getActiveTextureBind(GLenum target)
  {
    switch(target)
    {
    case GL_TEXTURE_1D: return activeTexture1d;
    case GL_TEXTURE_2D: return activeTexture2d;
    case GL_TEXTURE_3D: return activeTexture3d;
    case GL_TEXTURE_1D_ARRAY: return activeTexture1dArray;
    case GL_TEXTURE_2D_ARRAY: return activeTexture2dArray;
    case GL_TEXTURE_RECTANGLE: return activeTextureRectangle;
    case GL_TEXTURE_BUFFER: return activeTextureBuffer;
    case GL_TEXTURE_CUBE_MAP: return activeTextureCubeMap;
    case GL_TEXTURE_2D_MULTISAMPLE: return activeTexture2dMultisample;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return activeTexture2dMultisampleArray;
    default: jassertfalse; return *reinterpret_cast<GLRegister<GLuint>* >(NULL); // check isValidTextureBindingTarget before call
    }
  }

  // pixel pack: VRAM->RAM, unpack: RAM->VRAM
  virtual GLRegister<GLint>& getPixelStoreRowByteAligment(bool packing)
    {if (packing) return pixelStorePackRowByteAligment; else return pixelStoreUnPackRowByteAligment;}
  virtual GLRegister<GLint>& getPixelStorePaddedImageWidth(bool packing)
    {if (packing) return pixelStorePackPaddedImageWidth; else return pixelStoreUnPackPaddedImageWidth;}
  virtual GLRegister<GLint>& getPixelStorePaddedImageHeight(bool packing)
    {if (packing) return pixelStorePackPaddedImageHeight; else return pixelStoreUnPackPaddedImageHeight;}
  virtual GLRegister<GLboolean>& getPixelStoreHaveLittleEndianComponentBytesOrdering(bool packing)
    {if (packing) return pixelStorePackHaveLittleEndianComponentBytesOrdering; else return pixelStoreUnPackHaveLittleEndianComponentBytesOrdering;}
  virtual GLRegister<GLboolean>& getPixelStoreHaveReversedBitsOrdering(bool packing)
    {if (packing) return pixelStorePackHaveReversedBitsOrdering; else return pixelStoreUnPackHaveReversedBitsOrdering;}
  virtual GLRegister<GLint>& getPixelStoreNumSkippedPixels(bool packing)
    {if (packing) return pixelStorePackNumSkippedPixels; else return pixelStoreUnPackNumSkippedPixels;}
  virtual GLRegister<GLint>& getPixelStoreNumSkippedRows(bool packing)
    {if (packing) return pixelStorePackNumSkippedRows; else return pixelStoreUnPackNumSkippedRows;}
  virtual GLRegister<GLint>& getPixelStoreNumSkippedImages(bool packing)
    {if (packing) return pixelStorePackNumSkippedImages; else return pixelStoreUnPackNumSkippedImages;}
  virtual GLRegister<GLPixelStore>& getPixelStore(bool packing)
    {if (packing) return pixelStorePack; else return pixelStoreUnPack;}

  // buffer
  virtual bool isValidBufferBindingTarget(GLenum target) const
  {
    switch (target)
    {
    case GL_ARRAY_BUFFER:
    case GL_COPY_READ_BUFFER:
    case GL_COPY_WRITE_BUFFER:
    case GL_ELEMENT_ARRAY_BUFFER:
    case GL_PIXEL_PACK_BUFFER:
    case GL_PIXEL_UNPACK_BUFFER:
    case GL_TEXTURE_BUFFER:
    case GL_TRANSFORM_FEEDBACK_BUFFER:
    case GL_UNIFORM_BUFFER:
      return true;
    default:
      return false;
    }
  }
  virtual GLRegister<GLuint>& getActiveBufferBind(GLenum target)
  {
    switch(target)
    {
      case GL_ARRAY_BUFFER: return activeBufferForArray;
      case GL_COPY_READ_BUFFER: return activeBufferForCopyRead;
      case GL_COPY_WRITE_BUFFER: return activeBufferForCopyWrite;
      case GL_ELEMENT_ARRAY_BUFFER: return activeBufferForElementArray;
      case GL_PIXEL_PACK_BUFFER: return activeBufferForPixelPack;
      case GL_PIXEL_UNPACK_BUFFER: return activeBufferForPixelUnPack;
      case GL_TEXTURE_BUFFER: return activeBufferForTexture;
      case GL_TRANSFORM_FEEDBACK_BUFFER: return activeBufferForTransformFeedback;
      case GL_UNIFORM_BUFFER: return activeBufferForUniform;
      default: jassertfalse; return *reinterpret_cast<GLRegister<GLuint>* >(NULL); // check isValidBufferBindingTarget before call
    }
  }

  // vertex array
  virtual bool isValidVertexAttributeIndex(GLuint index)
    {return index < getNumVertexAttributes();}
  virtual GLuint getNumVertexAttributes() const
    {return integerConstantsStore.getValue(GL_MAX_VERTEX_ATTRIBS);}
  virtual GLRegister<GLuint>& getActiveVertexArrayBind()
    {return activeVertexArrayBind;}

  // program
  virtual bool isValidShaderType(GLenum shaderType) const
    {return shaderType == GL_VERTEX_SHADER || shaderType == GL_GEOMETRY_SHADER || shaderType == GL_FRAGMENT_SHADER
      || shaderType == GL_TESS_CONTROL_SHADER || shaderType == GL_TESS_EVALUATION_SHADER;} // for OpenGL 4.1 only

  virtual GLRegister<GLenum>& getActiveProgramBind()
    {return activeProgramBind;}

  virtual GLErrorFlags loadUniformSubroutines(GLenum shaderType, GLsizei count, const GLuint* indices)
  {
    if (!GLEW_ARB_shader_subroutine)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;} // // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    if (!isValidShaderType(shaderType))
      return GLErrorFlags::invalidEnumFlag;
    // TODO assert count == GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS
    // TODO assert indices[*] <= GL_ACTIVE_SUBROUTINES
    clearErrorFlags();
    glUniformSubroutinesuiv(shaderType, count, indices);
    const GLErrorFlags errorFlags = popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  virtual GLuint getMaxSubRoutines() const
    {return integerConstantsStore.getValue(GL_MAX_SUBROUTINES);}
  virtual GLuint getMaxSubRoutinesUniformLocation() const
    {return integerConstantsStore.getValue(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS);}

  // program pipeline
  virtual GLRegister<GLenum>& getActiveProgramPipelineBind()
    {return activeProgramPipelineBind;}

# ifdef GLEW_MX
  virtual GLEWContext* glewGetContext() const
    {return const_cast<GLEWContext*>(&glewContext);}
# endif //!GLEW_MX

protected:
  // Implementation Dependent globals
  bool initialized;
  GLIntegerConstantsStore integerConstantsStore;
  GLStringConstantsStore stringConstantsStore;
  GLFloatConstantsStore floatConstantsStore;

  // hints
  GLHint<GL_LINE_SMOOTH_HINT> lineSmoothHint;
  GLHint<GL_POLYGON_SMOOTH_HINT> polygonSmoothHint;
  GLHint<GL_TEXTURE_COMPRESSION_HINT> textureCompressionQualityHint;
  GLHint<GL_FRAGMENT_SHADER_DERIVATIVE_HINT> fragmentShaderDerivativeAccuracyHint;

  // clear
  GLClearColor clearColor;
  GLClearDepth clearDepth;
  GLClearStencil clearStencil;

  // viewport
  GLActiveViewport activeViewport;

  // scissor
  GLActiveScissor activeScissor;
  GLBooleanRegister<GL_SCISSOR_TEST> scissorTest;

  // depth
  GLBooleanRegister<GL_DEPTH_TEST> depthTest;

  // point
  GLBooleanRegister<GL_PROGRAM_POINT_SIZE> pointSizeProgrammable;
  GLPointSize pointSize;
  GLBooleanRegister<GL_POLYGON_OFFSET_POINT> pointPolygonOffset;

  // line
  GLBooleanRegister<GL_POLYGON_OFFSET_LINE> linePolygonOffset;

  // polygon
  GLBooleanRegister<GL_CULL_FACE> cullFace;
  GLBooleanRegister<GL_POLYGON_OFFSET_FILL> fillPolygonOffset;
  GLActivePolygonOffset polygonOffset;

  // multitexture
  GLActiveTextureUnit activeTextureUnit;

  // texture
  GLActiveTextureBind<GL_TEXTURE_1D,                   GL_TEXTURE_BINDING_1D>                   activeTexture1d;
  GLActiveTextureBind<GL_TEXTURE_2D,                   GL_TEXTURE_BINDING_2D>                   activeTexture2d;
  GLActiveTextureBind<GL_TEXTURE_3D,                   GL_TEXTURE_BINDING_3D>                   activeTexture3d;
  GLActiveTextureBind<GL_TEXTURE_1D_ARRAY,             GL_TEXTURE_BINDING_1D_ARRAY>             activeTexture1dArray;
  GLActiveTextureBind<GL_TEXTURE_2D_ARRAY,             GL_TEXTURE_BINDING_2D_ARRAY>             activeTexture2dArray;
  GLActiveTextureBind<GL_TEXTURE_RECTANGLE,            GL_TEXTURE_BINDING_RECTANGLE>            activeTextureRectangle;
  GLActiveTextureBind<GL_TEXTURE_BUFFER,               GL_TEXTURE_BINDING_BUFFER>               activeTextureBuffer;
  GLActiveTextureBind<GL_TEXTURE_CUBE_MAP,             GL_TEXTURE_BINDING_CUBE_MAP>             activeTextureCubeMap;
  GLActiveTextureBind<GL_TEXTURE_2D_MULTISAMPLE,       GL_TEXTURE_BINDING_2D_MULTISAMPLE>       activeTexture2dMultisample;
  GLActiveTextureBind<GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY> activeTexture2dMultisampleArray;

  // pixel
  GLPixelStoreSubRegister<GLint,     GL_PACK_ALIGNMENT, 4>   pixelStorePackRowByteAligment;
  GLPixelStoreSubRegister<GLint,     GL_PACK_ROW_LENGTH>     pixelStorePackPaddedImageWidth;
  GLPixelStoreSubRegister<GLint,     GL_PACK_IMAGE_HEIGHT>   pixelStorePackPaddedImageHeight;
  GLPixelStoreSubRegister<GLboolean, GL_PACK_SWAP_BYTES>     pixelStorePackHaveLittleEndianComponentBytesOrdering;
  GLPixelStoreSubRegister<GLboolean, GL_PACK_LSB_FIRST>      pixelStorePackHaveReversedBitsOrdering;
  GLPixelStoreSubRegister<GLint,     GL_PACK_SKIP_PIXELS>    pixelStorePackNumSkippedPixels;
  GLPixelStoreSubRegister<GLint,     GL_PACK_SKIP_ROWS>      pixelStorePackNumSkippedRows;
  GLPixelStoreSubRegister<GLint,     GL_PACK_SKIP_IMAGES>    pixelStorePackNumSkippedImages;
  GLPixelStoreSubRegister<GLint,     GL_UNPACK_ALIGNMENT, 4> pixelStoreUnPackRowByteAligment;
  GLPixelStoreSubRegister<GLint,     GL_UNPACK_ROW_LENGTH>   pixelStoreUnPackPaddedImageWidth;
  GLPixelStoreSubRegister<GLint,     GL_UNPACK_IMAGE_HEIGHT> pixelStoreUnPackPaddedImageHeight;
  GLPixelStoreSubRegister<GLboolean, GL_UNPACK_SWAP_BYTES>   pixelStoreUnPackHaveLittleEndianComponentBytesOrdering;
  GLPixelStoreSubRegister<GLboolean, GL_UNPACK_LSB_FIRST>    pixelStoreUnPackHaveReversedBitsOrdering;
  GLPixelStoreSubRegister<GLint,     GL_UNPACK_SKIP_PIXELS>  pixelStoreUnPackNumSkippedPixels;
  GLPixelStoreSubRegister<GLint,     GL_UNPACK_SKIP_ROWS>    pixelStoreUnPackNumSkippedRows;
  GLPixelStoreSubRegister<GLint,     GL_UNPACK_SKIP_IMAGES>  pixelStoreUnPackNumSkippedImages;
private: // register alias are private to deny a try from client to cache them
  GLPixelStoreRegister<true> pixelStorePack;
  GLPixelStoreRegister<false> pixelStoreUnPack;
protected:

  // buffer
  GLActiveBufferBind<GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING>                            activeBufferForArray;
  GLActiveBufferBind<GL_COPY_READ_BUFFER>                                                 activeBufferForCopyRead;
  GLActiveBufferBind<GL_COPY_READ_BUFFER>                                                 activeBufferForCopyWrite;
  GLActiveBufferBind<GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING>            activeBufferForElementArray;
  GLActiveBufferBind<GL_PIXEL_PACK_BUFFER, GL_PIXEL_PACK_BUFFER_BINDING>                  activeBufferForPixelPack;
  GLActiveBufferBind<GL_PIXEL_UNPACK_BUFFER, GL_PIXEL_UNPACK_BUFFER_BINDING>              activeBufferForPixelUnPack;
  GLActiveBufferBind<GL_TEXTURE_BUFFER, GL_TEXTURE_BINDING_BUFFER>                        activeBufferForTexture;
  GLActiveBufferBind<GL_TRANSFORM_FEEDBACK_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING>  activeBufferForTransformFeedback;
  GLActiveBufferBind<GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING>                        activeBufferForUniform;

  // vertex array
  GLActiveVertexArrayBind activeVertexArrayBind;

  // program
  GLActiveProgramBind activeProgramBind;

  // program pipeline
  GLActiveProgramPipelineBind activeProgramPipelineBind;

  // glew multicontext abstraction
# ifdef GLEW_MX
  GLEWContext glewContext;
# endif // GLEW_MX
};

//////////////////////////////////////////////////////////////////////////////

struct GLPackedImage
{
  GLPackedImage()
    : data(0) {}

  // manual initializing
  void set(GLenum format, GLenum type, GLvoid* data)
  {
    this->format = format;
    this->type = type;
    this->data = data;
    jassert(isValid());
  }

  // floating initializing helper
  void setR(GLfloat* data)
    {set(GL_RED, GL_FLOAT, data);}
  void setRG(GLfloat* data)
    {set(GL_RG, GL_FLOAT, data);}
  void setRGB(GLfloat* data)
    {set(GL_RGB, GL_FLOAT, data);}
  void setRGBA(GLfloat* data)
    {set(GL_RGBA, GL_FLOAT, data);}
  void setBGRA(GLfloat* data)
    {set(GL_BGRA, GL_FLOAT, data);}

  // unsigned byte initializing helper
  void setR(GLubyte* data)
    {set(GL_RED, GL_UNSIGNED_BYTE, data);}
  void setRG(GLubyte* data)
    {set(GL_RG, GL_UNSIGNED_BYTE, data);}
  void setRGB(GLubyte* data)
    {set(GL_RGB, GL_UNSIGNED_BYTE, data);}
  void setRGBA(GLubyte* data)
    {set(GL_RGBA, GL_UNSIGNED_BYTE, data);}
  void setBGRA(GLubyte* data)
    {set(GL_BGRA, GL_UNSIGNED_BYTE, data);}

  // Validation
  static bool isValidFormat(GLenum format) // extend if needed
    {return format == GL_RED || format == GL_RG || format == GL_RGB || format == GL_RGBA || format == GL_BGRA;}
  static bool isValidType(GLenum type) // extend if needed
    {return type == GL_FLOAT || type == GL_UNSIGNED_BYTE;}
  bool isValid() const
  {
    bool valid = pixelStore.isValid();
    if (!valid)
      {jassertfalse; return false;}
    valid = isValidFormat(format);
    if (!valid)
      {jassertfalse; return false;}
    valid = isValidType(type);
    if (!valid)
      {jassertfalse; return false;}
    valid = data != NULL;
    if (!valid)
      {jassertfalse; return false;}
    return valid;
  }

  // return component count or zero on error
  static size_t getNumComponents(GLenum format)
  {
    switch (format)
    {
    case GL_RED:
      return 1;
    case GL_RG:
      return 2;
    case GL_RGB:
      return 3;
    case GL_RGBA:
    case GL_BGRA:
      return 4;
    default:
      jassertfalse; // unknown or deprecated format: extend if needed.
      return 0;
    }
  }
  size_t getNumComponents() const
    {return getNumComponents(format);}

  // return pixel size in byte or zero on error
  static size_t getPixelSize(GLenum format, GLenum type)
  {
    const size_t numComponent = getNumComponents(format);
    switch (type)
    {
    case GL_FLOAT: return numComponent * sizeof(GLfloat);
    case GL_UNSIGNED_BYTE: return numComponent * sizeof(GLubyte);
    default:
      jassertfalse; // unknown or deprecated type: extend if needed.
      return 0;
    }
  }
  size_t getPixelSize() const
    {return getPixelSize(format, type);}

  GLPixelStore pixelStore;
  GLenum  format;
  GLenum  type;
  GLvoid* data;
};

//////////////////////////////////////////////////////////////////////////////

class GLObject
{
public:
  GLObject(GLContext& context)
    : context(context), id(0) {}

  virtual ~GLObject()
  {
    jassert(id == 0) // VRAM leak? Call destroy() or setId(0) on your object before destruction.
    // TODO check binding
  }

  // Construct custom object (check isValid after a non-zero set)
  // To discard destructor assert when object is used as a 'reference', unlink object from openGL named Object with setId(0)
  void setId(GLuint id)
    {this->id = id;}

  void destroy()
  {
    if (id)
    {
      // TODO check binding
      jassert(isValid()); // not a valid gl object. RAM corruption or deleted from another object or context.
      deleteNames(id);
      jassertglsucceed(context);
      id = 0;
    }
    else
      {jassertfalse;} // try to destroy an uncreated named object
  }

  GLuint getId() const
    {return id;}

  GLContext& getContext() const
    {return context;}

  // Warning: can produce OpenGL flushing performance penalty.
  bool isValid() const
    {return id && (isValidName(id) != GL_FALSE);} // glIsXXX function never produce errorFlags

# ifdef GLEW_MX
  GLEWContext* glewGetContext() const
    {return context.glewGetContext();}
# endif // GLEW_MX

protected:
  // interface
  virtual GLboolean isValidName(GLuint id) const = 0;
  virtual void deleteNames(GLuint id) const = 0;

protected:
  GLContext& context;
  GLuint id;
};

//////////////////////////////////////////////////////////////////////////////

// WARNING: ensure GLRegisterType::setValue can never fail (debug assert only)
// WARNING: use a cached register to do not perform extra setValue if there is no changes to do.
// WARNING: not using a cached register can produce OpenGL flush performance penalty when reading previous value.
template <class GLScopedSetValueType>
class GLScopedSetValue
{
public:
  typedef GLScopedSetValueType GLType;
  typedef GLRegister<GLType> GLRegisterType;

  GLScopedSetValue(GLRegisterType& reg, const GLType newValue)
    : reg(reg)
  {
    constructionState = reg.getValue(backupValue);
    if (constructionState.hasErrors())
      {jassertfalse; return;}
    constructionState = reg.setValue(newValue);
    jassert(constructionState.hasSucceed());
  }

  void cancelScopedValue() const
  {
    if (constructionState.hasSucceed())
    {
      bool succeed = reg.setValue(backupValue).hasSucceed();
      jassert(succeed);
      succeed = succeed; // avoid unused value warning on release.
    }
  }

  GLErrorFlags getConstructionState() const
    {return constructionState;}

 ~GLScopedSetValue()
 {
   jassertglsucceed(reg.getContext()); // ensure popping errors before scope exit to do not produce cancelScopedValue false assertion.
   cancelScopedValue();
 }

private:
  GLErrorFlags constructionState;
  GLRegisterType& reg;
  GLType backupValue;
};

//////////////////////////////////////////////////////////////////////////////

class GLTextureObject : public GLObject
{
public:
  GLTextureObject(GLContext& context)
    : GLObject(context), target(0) {}

  GLenum getTarget() const
    {return target;}

  // Texture properties Warning: possible OpenGL flush performance penalty.
  GLsizei getWidth(GLint level = 0) const;
  GLsizei getHeight(GLint level = 0) const;
  GLsizei getDepth(GLint level = 0) const;
  GLsizei getNumSamplesPerTexel(GLint level = 0) const; // return 0 if texture is not multisampled
  GLint getInternalFormat(GLint level = 0) const;
  GLsizei getNumBitsForRed(GLint level = 0) const;
  GLsizei getNumBitsForGreen(GLint level = 0) const;
  GLsizei getNumBitsForBlue(GLint level = 0) const;
  GLsizei getNumBitsForAlpha(GLint level = 0) const;
  GLsizei getNumBitsForDepth(GLint level = 0) const;
  GLsizei getNumBitsForStencil(GLint level = 0) const;
  GLboolean isCompressed(GLint level = 0) const;
  GLsizei getCompressedImageSize(GLint level = 0) const;

  // texture sampling parameter Warning: possible OpenGL flush performance penalty.
  GLErrorFlags getBaseLevel(GLint& baseLevel) const;
  GLErrorFlags getMaxLevel(GLint& maxLevel) const;
  GLErrorFlags getCompareMode(GLenum& compareMode) const;
  GLErrorFlags getCompareFunction(GLenum& compareFunction) const;
  GLErrorFlags getLevelOfDetailBias(GLfloat& levelOfDetailBias) const;
  GLErrorFlags getLevelOfDetailMax(GLfloat& levelOfDetailMax) const;
  GLErrorFlags getLevelOfDetailMin(GLfloat& levelOfDetailMin) const;
  GLErrorFlags getMagnificationFilter(GLenum& magnificationFilter) const;
  GLErrorFlags getMinificationFilter(GLenum& minificationFilter) const;
  GLErrorFlags getSwizzleRed(GLenum& swizzleRed) const;
  GLErrorFlags getSwizzleGreen(GLenum& swizzleGreen) const;
  GLErrorFlags getSwizzleBlue(GLenum& swizzleBlue) const;
  GLErrorFlags getSwizzleAlpha(GLenum& swizzleAlpha) const;
  GLErrorFlags getHorizontalWrapMode(GLenum& horizontalWrapMode) const;
  GLErrorFlags getVerticalWrapMode(GLenum& verticalWrapMode) const;
  GLErrorFlags getDepthWrapMode(GLenum& depthWrapMode) const;

  // texture sampling parameter modifier:
  GLErrorFlags setBaseLevel(GLint baseLevel);
  GLErrorFlags setMaxLevel(GLint maxLevel);
  GLErrorFlags setCompareMode(GLenum compareMode);
  GLErrorFlags setCompareFunction(GLenum compareFunction);
  GLErrorFlags setLevelOfDetailBias(GLfloat levelOfDetailBias);
  GLErrorFlags setLevelOfDetailMax(GLfloat levelOfDetailMax);
  GLErrorFlags setLevelOfDetailMin(GLfloat levelOfDetailMin);
  GLErrorFlags setMagnificationFilter(GLenum magnificationFilter);
  GLErrorFlags setMinificationFilter(GLenum minificationFilter);
  GLErrorFlags setSwizzleRed(GLenum swizzleRed);
  GLErrorFlags setSwizzleGreen(GLenum swizzleGreen);
  GLErrorFlags setSwizzleBlue(GLenum swizzleBlue);
  GLErrorFlags setSwizzleAlpha(GLenum swizzleAlpha);
  GLErrorFlags setHorizontalWrapMode(GLenum horizontalWrapMode);
  GLErrorFlags setVerticalWrapMode(GLenum verticalWrapMode);
  GLErrorFlags setDepthWrapMode(GLenum depthWrapMode);

  // Warning: possible OpenGL flush performance penalty.
  GLErrorFlags create2D(GLint internalFormat, GLsizei	width, GLsizei height, const GLPackedImage* data)
  {
    // todo jassert on internalFormat
    jassert(!id); // overwritting existing: potential memory leak
    jassert(context.isValidTextureSize(width) && context.isValidTextureSize(height));
    glGenTextures(1, &id);
    jassertglsucceed(context);
    jassert(id);

    target = GL_TEXTURE_2D;
    GLScopedSetValue<GLenum> _(context.getActiveTextureBind(target), id);
    jassertglsucceed(context);
    const GLenum nullDataFormat = getNULLDataFormat(internalFormat);
    // Todo optim? cache proxy result if non proxy succeed.
    //glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_UNSIGNED_BYTE, nullDataFormat, NULL)
    GLErrorFlags errorFlags;
    if (data)
    {
      GLScopedSetValue<GLPixelStore> __(context.getPixelStore(false), data->pixelStore);
      context.clearErrorFlags(); // clear previous error ensure next popContextErrorFlags concern glTexImage

      glTexImage2D(target, 0, internalFormat, width, height, 0, data->format, data->type, data->data);
      errorFlags = context.popErrorFlags(); // pop error before scope exit
    }
    else
    {
      context.clearErrorFlags(); // clear previous error ensure next popContextErrorFlags concern glTexImage
      glTexImage2D(target, 0, internalFormat, width, height, 0, GL_UNSIGNED_BYTE, nullDataFormat, NULL);
      errorFlags = context.popErrorFlags();
    }
    if (errorFlags.hasErrors())
    {
      _.cancelScopedValue(); // unbind texture id before deletion seem's to be cleaner
      destroy();
    }
    return errorFlags;
  }

protected:
  static GLenum getNULLDataFormat(GLint internalFormat)
  {
    return (internalFormat == GL_DEPTH_COMPONENT ||
            internalFormat == GL_DEPTH_COMPONENT16 ||
            internalFormat == GL_DEPTH_COMPONENT24)
        ? GL_DEPTH_COMPONENT : GL_RGBA;
  }

  static GLenum getTargetProxy(GLenum target)
  {
    switch (target)
    {
    case GL_TEXTURE_2D:
      return GL_PROXY_TEXTURE_2D;
    case GL_TEXTURE_1D_ARRAY:
      return GL_PROXY_TEXTURE_1D_ARRAY;
    case GL_TEXTURE_RECTANGLE:
      return GL_PROXY_TEXTURE_RECTANGLE;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return GL_PROXY_TEXTURE_CUBE_MAP;
    default: jassertfalse; return 0; // unknown texture target
    }
  }

  static bool isValidCompareMode(GLenum compareMode)
    {return compareMode == GL_NONE || compareMode == GL_COMPARE_REF_TO_TEXTURE;}

  static bool isValidMagnificationFilter(GLenum magnificationFilter)
    {return magnificationFilter == GL_NEAREST || magnificationFilter == GL_LINEAR;}

  static bool isValidMinificationFilter(GLenum minificationFilter)
  {
    return minificationFilter == GL_NEAREST ||
      minificationFilter == GL_LINEAR ||
      minificationFilter == GL_NEAREST_MIPMAP_NEAREST ||
      minificationFilter == GL_NEAREST_MIPMAP_LINEAR ||
      minificationFilter == GL_LINEAR_MIPMAP_NEAREST ||
      minificationFilter == GL_LINEAR_MIPMAP_LINEAR;
  }

  static bool isValidSwizzling(GLenum swizzling)
  {
    return swizzling == GL_RED || swizzling == GL_GREEN ||
      swizzling == GL_BLUE || swizzling == GL_ALPHA ||
      swizzling == GL_ZERO || swizzling == GL_ONE;
  }

  static bool isValidWrappingMode(GLenum wrappingMode)
    {return wrappingMode == GL_CLAMP_TO_EDGE || wrappingMode == GL_REPEAT ||
      wrappingMode == GL_CLAMP_TO_BORDER || wrappingMode == GL_MIRRORED_REPEAT;}

protected:
  // GLObject interface
  virtual GLboolean isValidName(GLuint id) const
    {return glIsTexture(id);}
  virtual void deleteNames(GLuint id) const
    {glDeleteTextures(1, &id);}

private:
  GLenum target;
};

//////////////////////////////////////////////////////////////////////////////

template <class GLTextureParameterType, GLenum parameterName, GLint defaultValue = 0>
class GLTextureParameterRegister : public GLRegister<GLTextureParameterType>
{
public:
  typedef GLTextureParameterType GLType;

  GLTextureParameterRegister(const GLTextureObject& textureObject)
    : GLRegister<GLType>(textureObject.getContext()), textureObject(textureObject) {}

  virtual GLType getDefaultValue() const
    {return static_cast<GLType>(defaultValue);}

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    const GLenum target = textureObject.getTarget();
    if (!GLRegister<GLType>::context.isValidTextureBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    GLRegister<GLType>::context.clearErrorFlags();
    GLScopedSetValue<GLuint> _(GLRegister<GLType>::context.getActiveTextureBind(target), textureObject.getId());
    GLErrorFlags errorFlags = _.getConstructionState();
    GLint intValue = defaultValue;
    if (errorFlags.hasSucceed())
    {
      glGetTexParameteriv(target, parameterName, &intValue);
      errorFlags = GLRegister<GLType>::context.popErrorFlags();
      if (errorFlags.hasSucceed())
      {
        if (intValue < 0)
          {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
      }
      else
        {jassertfalse;}
    }
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

  virtual GLErrorFlags setValue(const GLType& param)
  {
    // TODO check
    //if (!context.isValidTextureParameter(parameterName, param))
    //  {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    if (param < 0)
		  return GLErrorFlags::invalidValueFlag;
    const GLenum target = textureObject.getTarget();
    if (!GLRegister<GLType>::context.isValidTextureBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    GLScopedSetValue<GLuint> _(GLRegister<GLType>::context.getActiveTextureBind(target), textureObject.getId());

    glTexParameteri(target, parameterName, param);
    jassertglsucceed(GLRegister<GLType>::context);
    return GLErrorFlags::succeed;
  }

private:
   const GLTextureObject& textureObject;
};

template <GLenum parameterName, GLint defaultValue = 0>
class GLTextureFloatParameterRegister : public GLRegister<GLfloat>
{
public:
  typedef GLfloat GLType;

  GLTextureFloatParameterRegister(const GLTextureObject& textureObject)
    : GLRegister<GLfloat>(textureObject.getContext()), textureObject(textureObject) {}

  virtual GLfloat getDefaultValue() const
    {return defaultValue;}

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLfloat& param) const
  {
    const GLenum target = textureObject.getTarget();
    if (!context.isValidTextureBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    context.clearErrorFlags();
    GLScopedSetValue<GLuint> _(context.getActiveTextureBind(target), textureObject.getId());
    GLErrorFlags errorFlags = _.getConstructionState();
    GLfloat floatValue = defaultValue;
    if (errorFlags.hasSucceed())
    {
      glGetTexParameterfv(target, parameterName, &floatValue);
      errorFlags = context.popErrorFlags();
      if (errorFlags.hasErrors())
        {jassertfalse;}
    }
    param = floatValue;
    return errorFlags;
  }

  virtual GLErrorFlags setValue(const GLfloat& param)
  {
    // TODO check
    //if (!context.isValidTextureParameter(parameterName, param))
    //  {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    const GLenum target = textureObject.getTarget();
    if (!context.isValidTextureBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    GLScopedSetValue<GLuint> _(context.getActiveTextureBind(target), textureObject.getId());

    glTexParameterf(target, parameterName, param);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

private:
   const GLTextureObject& textureObject;
};

GLErrorFlags GLTextureObject::getBaseLevel(GLint& baseLevel) const
  {return GLTextureParameterRegister<GLint, GL_TEXTURE_BASE_LEVEL>(*this).getValue(baseLevel);}
GLErrorFlags GLTextureObject::getMaxLevel(GLint& maxLevel) const
  {return GLTextureParameterRegister<GLint, GL_TEXTURE_MAX_LEVEL, 1000>(*this).getValue(maxLevel);}
GLErrorFlags GLTextureObject::getCompareMode(GLenum& compareMode) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_COMPARE_MODE, GL_NONE>(*this).getValue(compareMode);}
GLErrorFlags GLTextureObject::getCompareFunction(GLenum& compareFunction) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL>(*this).getValue(compareFunction);}
GLErrorFlags GLTextureObject::getLevelOfDetailBias(GLfloat& levelOfDetailBias) const
  {return GLTextureFloatParameterRegister<GL_TEXTURE_LOD_BIAS>(*this).getValue(levelOfDetailBias);}
GLErrorFlags GLTextureObject::getLevelOfDetailMax(GLfloat& levelOfDetailMax) const
  {return GLTextureFloatParameterRegister<GL_TEXTURE_MAX_LOD, 1000>(*this).getValue(levelOfDetailMax);}
GLErrorFlags GLTextureObject::getLevelOfDetailMin(GLfloat& levelOfDetailMin) const
  {return GLTextureFloatParameterRegister<GL_TEXTURE_MIN_LOD, -1000>(*this).getValue(levelOfDetailMin);}
GLErrorFlags GLTextureObject::getMagnificationFilter(GLenum& manificationFilter) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_MAG_FILTER, GL_LINEAR>(*this).getValue(manificationFilter);}
GLErrorFlags GLTextureObject::getMinificationFilter(GLenum& minificationFilter) const
{
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_MIN_FILTER, GL_LINEAR>(*this).getValue(minificationFilter);
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR>(*this).getValue(minificationFilter);
}
GLErrorFlags GLTextureObject::getSwizzleRed(GLenum& swizzleRed) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_R, GL_RED>(*this).getValue(swizzleRed);}
GLErrorFlags GLTextureObject::getSwizzleGreen(GLenum& swizzleGreen) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_G, GL_GREEN>(*this).getValue(swizzleGreen);}
GLErrorFlags GLTextureObject::getSwizzleBlue(GLenum& swizzleBlue) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_B, GL_BLUE>(*this).getValue(swizzleBlue);}
GLErrorFlags GLTextureObject::getSwizzleAlpha(GLenum& swizzleAlpha) const
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_A, GL_ALPHA>(*this).getValue(swizzleAlpha);}
GLErrorFlags GLTextureObject::getHorizontalWrapMode(GLenum& horizontalWrapMode) const
{
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE>(*this).getValue(horizontalWrapMode);
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_S, GL_REPEAT>(*this).getValue(horizontalWrapMode);
}
GLErrorFlags GLTextureObject::getVerticalWrapMode(GLenum& verticalWrapMode) const
{
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE>(*this).getValue(verticalWrapMode);
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_T, GL_REPEAT>(*this).getValue(verticalWrapMode);
}
GLErrorFlags GLTextureObject::getDepthWrapMode(GLenum& depthWrapMode) const
{
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE>(*this).getValue(depthWrapMode);
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_R, GL_REPEAT>(*this).getValue(depthWrapMode);
}

GLErrorFlags GLTextureObject::setBaseLevel(GLint baseLevel)
  {return GLTextureParameterRegister<GLint, GL_TEXTURE_BASE_LEVEL>(*this).setValue(baseLevel);}
GLErrorFlags GLTextureObject::setMaxLevel(GLint maxLevel)
  {return GLTextureParameterRegister<GLint, GL_TEXTURE_MAX_LEVEL, 1000>(*this).setValue(maxLevel);}

GLErrorFlags GLTextureObject::setCompareMode(GLenum compareMode)
{
  if (!isValidCompareMode(compareMode))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  return GLTextureParameterRegister<GLenum, GL_TEXTURE_COMPARE_MODE, GL_NONE>(*this).setValue(compareMode);
}

GLErrorFlags GLTextureObject::setCompareFunction(GLenum compareFunction)
  {return GLTextureParameterRegister<GLenum, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL>(*this).setValue(compareFunction);}
GLErrorFlags GLTextureObject::setLevelOfDetailBias(GLfloat levelOfDetailBias)
  {return GLTextureFloatParameterRegister<GL_TEXTURE_LOD_BIAS>(*this).setValue(levelOfDetailBias);}
GLErrorFlags GLTextureObject::setLevelOfDetailMax(GLfloat levelOfDetailMax)
  {return GLTextureFloatParameterRegister<GL_TEXTURE_MAX_LOD, 1000>(*this).setValue(levelOfDetailMax);}
GLErrorFlags GLTextureObject::setLevelOfDetailMin(GLfloat levelOfDetailMin)
  {return GLTextureFloatParameterRegister<GL_TEXTURE_MIN_LOD, -1000>(*this).setValue(levelOfDetailMin);}
GLErrorFlags GLTextureObject::setMagnificationFilter(GLenum magnificationFilter)
{
  if (!isValidMagnificationFilter(magnificationFilter))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  return GLTextureParameterRegister<GLenum, GL_TEXTURE_MAG_FILTER, GL_LINEAR>(*this).setValue(magnificationFilter);
}
GLErrorFlags GLTextureObject::setMinificationFilter(GLenum minificationFilter)
{
  if (!isValidMinificationFilter(minificationFilter))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}

  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_MIN_FILTER, GL_LINEAR>(*this).setValue(minificationFilter); // TODO assert if mimapping filtering
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR>(*this).setValue(minificationFilter);
}
GLErrorFlags GLTextureObject::setSwizzleRed(GLenum swizzleRed)
{
  if (!isValidSwizzling(swizzleRed))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_R, GL_RED>(*this).setValue(swizzleRed);
}
GLErrorFlags GLTextureObject::setSwizzleGreen(GLenum swizzleGreen)
{
  if (!isValidSwizzling(swizzleGreen))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_G, GL_GREEN>(*this).setValue(swizzleGreen);
}
GLErrorFlags GLTextureObject::setSwizzleBlue(GLenum swizzleBlue)
{
  if (!isValidSwizzling(swizzleBlue))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_B, GL_BLUE>(*this).setValue(swizzleBlue);
}
GLErrorFlags GLTextureObject::setSwizzleAlpha(GLenum swizzleAlpha)
{
  if (!isValidSwizzling(swizzleAlpha))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  return GLTextureParameterRegister<GLenum, GL_TEXTURE_SWIZZLE_A, GL_ALPHA>(*this).setValue(swizzleAlpha);
}
GLErrorFlags GLTextureObject::setHorizontalWrapMode(GLenum horizontalWrapMode)
{
  if (!isValidWrappingMode(horizontalWrapMode))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE>(*this).setValue(horizontalWrapMode); // assert if repeat
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_S, GL_REPEAT>(*this).setValue(horizontalWrapMode);
}
GLErrorFlags GLTextureObject::setVerticalWrapMode(GLenum verticalWrapMode)
{
  if (!isValidWrappingMode(verticalWrapMode))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE>(*this).setValue(verticalWrapMode); // assert if repeat
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_T, GL_REPEAT>(*this).setValue(verticalWrapMode);
}

GLErrorFlags GLTextureObject::setDepthWrapMode(GLenum depthWrapMode)
{
  if (!isValidWrappingMode(depthWrapMode))
    {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
  if (target == GL_TEXTURE_RECTANGLE)
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE>(*this).setValue(depthWrapMode); // assert if repeat
  else
    return GLTextureParameterRegister<GLenum, GL_TEXTURE_WRAP_R, GL_REPEAT>(*this).setValue(depthWrapMode);
}

//////////////////////////////////////////////////////////////////////////////

template <class GLTexturePropertyType, GLenum propertyName>
class GLTextureProperty : public GLProperty<GLTexturePropertyType>
{
public:
  typedef GLTexturePropertyType GLType;

  GLTextureProperty(const GLTextureObject& textureObject, GLint level)
    : GLProperty<GLType>(textureObject.getContext()), textureObject(textureObject), level(level)
    {
      jassert(level >= 0);
      jassert(textureObject.isValid());
    }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLType getValue() const
  {
    const GLenum target = textureObject.getTarget();
    jassert(GLProperty<GLType>::context.isValidTextureBindingTarget(target));
    GLScopedSetValue<GLuint> _(GLProperty<GLType>::context.getActiveTextureBind(target), textureObject.getId());
    GLint intValue = 0;
    glGetTexLevelParameteriv(target, level, propertyName, &intValue);
    jassertglsucceed(GLProperty<GLType>::context);
    jassert(intValue >= 0);
    return static_cast<GLType>(intValue);
  }

  virtual GLErrorFlags getValue(GLType& param) const
  {
    const GLenum target = textureObject.getTarget();
    if (!GLProperty<GLType>::context.isValidTextureBindingTarget(target))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    GLProperty<GLType>::context.clearErrorFlags();
    GLScopedSetValue<GLuint> _(GLProperty<GLType>::context.getActiveTextureBind(target), textureObject.getId());
    GLErrorFlags errorFlags = _.getConstructionState();
    GLint intValue = 0;
    if (errorFlags.hasSucceed())
    {
      glGetTexLevelParameteriv(target, level, propertyName, &intValue);
      errorFlags = GLProperty<GLType>::context.popErrorFlags();
      if (errorFlags.hasSucceed())
      {
        if (intValue < 0)
          {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
      }
      else
        {jassertfalse;}
    }
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

private:
   const GLTextureObject& textureObject;
   const GLint level;
};

// GLTextureObject
GLsizei GLTextureObject::getWidth(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_WIDTH>(*this, level).getValue();}
GLsizei GLTextureObject::getHeight(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_HEIGHT>(*this, level).getValue();}
GLsizei GLTextureObject::getDepth(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_DEPTH>(*this, level).getValue();}
GLsizei GLTextureObject::getNumSamplesPerTexel(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_SAMPLES>(*this, level).getValue();}
GLint GLTextureObject::getInternalFormat(GLint level) const
  {return GLTextureProperty<GLint, GL_TEXTURE_INTERNAL_FORMAT>(*this, level).getValue();}
GLsizei GLTextureObject::getNumBitsForRed(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_RED_SIZE>(*this, level).getValue();}
GLsizei GLTextureObject::getNumBitsForGreen(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_GREEN_SIZE>(*this, level).getValue();}
GLsizei GLTextureObject::getNumBitsForBlue(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_BLUE_SIZE>(*this, level).getValue();}
GLsizei GLTextureObject::getNumBitsForAlpha(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_ALPHA_SIZE>(*this, level).getValue();}
GLsizei GLTextureObject::getNumBitsForDepth(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_DEPTH_SIZE>(*this, level).getValue();}
GLsizei GLTextureObject::getNumBitsForStencil(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_STENCIL_SIZE>(*this, level).getValue();}
GLboolean GLTextureObject::isCompressed(GLint level) const
  {return GLTextureProperty<GLboolean, GL_TEXTURE_COMPRESSED>(*this, level).getValue();}
GLsizei GLTextureObject::getCompressedImageSize(GLint level) const
  {return GLTextureProperty<GLsizei, GL_TEXTURE_COMPRESSED_IMAGE_SIZE>(*this, level).getValue();}
// todo add TEXTURE_FIXED_SAMPLE_LOCATIONS, TEXTURE_BUFFER_DATA_STORE_BINDING...

//////////////////////////////////////////////////////////////////////////////

class GLBufferObject : public GLObject
{
public:
  GLBufferObject(GLContext& context)
    : GLObject(context) {}

  // Warning: possible OpenGL flush performance penalty.
  GLErrorFlags create(GLsizeiptr size, const GLvoid* data, GLenum usage, GLenum temporaryTarget = GL_ARRAY_BUFFER)
  {
    jassert(!id); // overwritting existing: potential memory leak
    jassert(size >= 0);
    jassert(context.isValidBufferBindingTarget(temporaryTarget) && isValidUsage(usage));
    glGenBuffers(1, &id);
    jassertglsucceed(context);
    jassert(id);

    GLScopedSetValue<GLuint> _(context.getActiveBufferBind(temporaryTarget), id);
    context.clearErrorFlags(); // clear previous error ensure next popContextErrorFlags concern glBufferData
    glBufferData(temporaryTarget, size, data, usage);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
    {
      _.cancelScopedValue(); // unbind before deletion seem's to be cleaner
      destroy();
    }
    return errorFlags;
  }

  GLErrorFlags set(GLsizeiptr size, const GLvoid* data, GLintptr offset = 0, GLenum temporaryTarget = GL_ARRAY_BUFFER)
  {
    if (offset < 0 || size < 0)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!context.isValidBufferBindingTarget(temporaryTarget))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}

    GLScopedSetValue<GLuint> _(context.getActiveBufferBind(temporaryTarget), id);
    glBufferSubData(temporaryTarget, 0, size, data);
    jassertglsucceed(context); // invalidValueFlag if offset + size extends beyond the buffer object's allocated data store
                      // invalidOperationFlag if the buffer object being updated is mapped.
    return GLErrorFlags::succeed;
  }

  GLErrorFlags map(GLenum access, void** data, GLenum temporaryTarget = GL_ARRAY_BUFFER)
  {
    if (!data || !isValidAccess(access) || !context.isValidBufferBindingTarget(temporaryTarget))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    GLScopedSetValue<GLuint> _(context.getActiveBufferBind(temporaryTarget), id);
    *data = glMapBuffer(temporaryTarget, access);
    if (!*data)
    { // in this error case only, we could have performance penalty
      const GLErrorFlags errorFlags = context.popErrorFlags();
      jassert(errorFlags.hasErrors()); // hum hum: openGL say that if glMapBuffer return NULL, an error is generated
      // invalidOperationFlag: buffer object is already mapped
      // outOfVideoMemoryFlag: can be a virtual memory problem
      return errorFlags;
    }
    return GLErrorFlags::succeed;
  }

  GLErrorFlags unmap(GLenum temporaryTarget = GL_ARRAY_BUFFER)
  {
    if (!context.isValidBufferBindingTarget(temporaryTarget))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    GLScopedSetValue<GLuint> _(context.getActiveBufferBind(temporaryTarget), id);
    if (glUnmapBuffer(temporaryTarget))
      return GLErrorFlags::succeed;
    else
    {
      const GLErrorFlags errorFlags = context.popErrorFlags();
      jassert(errorFlags.hasErrors()); // hum hum: openGL say that if glMapBuffer return NULL, an error is generated
      // invalidOperationFlag: buffer object is not currently mapped
      return errorFlags;
    }
  }

  // TODO glGetBufferPointerv glGetBufferParameter

protected:
  // GLObject interface
  virtual GLboolean isValidName(GLuint id) const
    {return glIsBuffer(id);}
  virtual void deleteNames(GLuint id) const
    {glDeleteBuffers(1, &id);}

private:
  static bool isValidUsage(GLenum usage)
  {
    switch (usage)
    {
    case GL_STREAM_DRAW:
    case GL_STREAM_READ:
    case GL_STREAM_COPY:
    case GL_STATIC_DRAW:
    case GL_STATIC_READ:
    case GL_STATIC_COPY:
    case GL_DYNAMIC_DRAW:
    case GL_DYNAMIC_READ:
    case GL_DYNAMIC_COPY:
      return true;
    default:
      return false;
    }
  }

  static bool isValidAccess(GLenum access)
    {return access == GL_READ_ONLY || access == GL_WRITE_ONLY || access == GL_READ_WRITE;}
};

//////////////////////////////////////////////////////////////////////////////

class GLVertexArrayObject : public GLObject
{
public:
  GLVertexArrayObject(GLContext& context)
    : GLObject(context) {}

  GLErrorFlags create()
  {
    jassert(!id); // overwritting existing: potential memory leak
    glGenVertexArrays(1, &id);
    jassertglsucceed(context);
    jassert(id);

    GLScopedSetValue<GLuint> _(context.getActiveVertexArrayBind(), id);
    jassertglsucceed(context);
    // todo others user friendly things before leaving
    return context.popErrorFlags();
  }

  static bool isValidComponentPerVertexAttributeCount(GLint count)
    {return (count >= 1 && count <= 4) || (count == GL_BGRA);}

  GLErrorFlags linkAttributeToBuffer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLuint bufferId)
  {
    // todo check type
    if (!context.isValidVertexAttributeIndex(index) || !isValidComponentPerVertexAttributeCount(size) || !bufferId)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    GLScopedSetValue<GLuint> _(context.getActiveVertexArrayBind(), id);
    glEnableVertexAttribArray(index);
    jassertglsucceed(context);

    GLScopedSetValue<GLuint> __(context.getActiveBufferBind(GL_ARRAY_BUFFER), bufferId);
    glVertexAttribPointer(index, size, type, normalized, 0, 0);
    jassertglsucceed(context);

    return GLErrorFlags::succeed;
  }

  GLErrorFlags unLinkAttribute(GLuint index)
  {
    if (!context.isValidVertexAttributeIndex(index))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    glDisableVertexAttribArray(index);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // todo glGetVertexAttrib(GL_VERTEX_ATTRIB_ARRAY_ENABLED) glVertexAttrib

  GLErrorFlags draw(GLenum mode, GLint first, GLsizei count)
  {
    jassert(isValid());
    GLScopedSetValue<GLuint> _(context.getActiveVertexArrayBind(), id);
    glDrawArrays(mode, first, count);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

protected:
  // GLObject interface
  virtual GLboolean isValidName(GLuint id) const
    {return glIsVertexArray(id);}
  virtual void deleteNames(GLuint id) const
    {glDeleteVertexArrays(1, &id);}
};

//////////////////////////////////////////////////////////////////////////////

class GLLoggedInterface
{
public:
  virtual GLErrorFlags getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const = 0;
  virtual GLErrorFlags getLastLog(GLsizei maxLength, GLsizei* length, GLchar* infoLog) const = 0;
};

class GLBuildableInterface : public GLLoggedInterface
{
public:
  virtual GLErrorFlags build() = 0;
  virtual GLErrorFlags hasBuildSucceed(GLboolean& buildSucceed) const = 0;
};

class GLValidableInterface : public GLLoggedInterface
{
public:
  virtual GLErrorFlags validate() = 0;
  virtual GLErrorFlags hasValidationSucceed(GLboolean& validationSucceed) const = 0;
};

//////////////////////////////////////////////////////////////////////////////

class GLShaderObject : public GLObject, public GLBuildableInterface
{
public:
  GLShaderObject(GLContext& context)
    : GLObject(context) {}

  GLErrorFlags create(GLenum shaderType, GLsizei count, const GLchar** source, const GLint* length = 0)
  {
    jassert(!id); // overwritting existing: potential memory leak
    // optimizing: here we no not need to access to OpenGL errorFlags.
    if (count < 0)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!context.isValidShaderType(shaderType))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    id = glCreateShader(shaderType);
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    jassertglsucceed(context);
    glShaderSource(id, count, source, length);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  };

  virtual GLErrorFlags build()
  {
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    glCompileShader(id);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // properties Warning: possible OpenGL flush performance penalty.
  GLErrorFlags getType(GLenum& shaderType) const;
  GLErrorFlags wasFlaggedForDeletion(GLboolean& flaggedForDeletion) const;

  GLErrorFlags getNumCharacteresOfSource(GLsizei& numCharacteresOfSource) const;
  virtual GLErrorFlags hasBuildSucceed(GLboolean& buildSucceed) const;
  virtual GLErrorFlags getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const;
  virtual GLErrorFlags getLastLog(GLsizei maxLength, GLsizei* length, GLchar* infoLog) const
  {
    if (length)
      *length = 0;
    if (maxLength < 0 || !infoLog)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    jassert(isValid());
    context.clearErrorFlags();
    glGetShaderInfoLog(id, maxLength, length, infoLog);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }
  // TODO glGetShaderSource

protected:
  // GLObject interface
  virtual GLboolean isValidName(GLuint id) const
    {return glIsShader(id);}
  virtual void deleteNames(GLuint id) const
    {glDeleteShader(id);}
};

template <class GLType, GLenum propertyName>
class GLShaderProperty : public GLProperty<GLType>
{
public:
  GLShaderProperty(const GLShaderObject& shaderObject)
    : GLProperty<GLType>(shaderObject.getContext()), shaderObject(shaderObject)
    {jassert(shaderObject.isValid());}

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    GLint intValue = 0;
    GLProperty<GLType>::context.clearErrorFlags();
    glGetShaderiv(shaderObject.getId(), propertyName, &intValue);
    GLErrorFlags errorFlags =  GLProperty<GLType>::context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
     {jassertfalse;}
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

private:
   const GLShaderObject& shaderObject;
};

// GLShaderObject
GLErrorFlags GLShaderObject::getType(GLenum& shaderType) const
  {return GLShaderProperty<GLenum, GL_SHADER_TYPE>(*this).getValue(shaderType);}
GLErrorFlags GLShaderObject::wasFlaggedForDeletion(GLboolean& flaggedForDeletion) const
  {return GLShaderProperty<GLboolean, GL_DELETE_STATUS>(*this).getValue(flaggedForDeletion);}
GLErrorFlags GLShaderObject::hasBuildSucceed(GLboolean& buildSucceed) const
  {return GLShaderProperty<GLboolean, GL_COMPILE_STATUS>(*this).getValue(buildSucceed);}
GLErrorFlags GLShaderObject::getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const
  {return GLShaderProperty<GLsizei, GL_INFO_LOG_LENGTH>(*this).getValue(numCharacteresOfLastBuildLog);}
GLErrorFlags GLShaderObject::getNumCharacteresOfSource(GLsizei& numCharacteresOfSource) const
  {return GLShaderProperty<GLsizei, GL_SHADER_SOURCE_LENGTH>(*this).getValue(numCharacteresOfSource);}

//////////////////////////////////////////////////////////////////////////////

// Predeclaration
template <class GLProgramUniformRegisterType, GLint size>
class GLProgramUniformRegister;
template <GLint numRows, GLint numColumns>
class GLProgramUniformMatrixRegister;

class GLProgramObject : public GLObject, public GLBuildableInterface, public GLValidableInterface
{
public:
  GLProgramObject(GLContext& context)
    : GLObject(context) {}

  GLErrorFlags create()
  {
    jassert(!id); // overwritting existing: potential memory leak
    // optimizing: here we no not need to access to OpenGL errorFlags.
    id = glCreateProgram();
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  };

  GLErrorFlags createSeparable(GLenum shaderType, GLsizei count, const GLchar** source)
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;} // // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    jassert(!id); // overwritting existing: potential memory leak
    if (count < 0)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!context.isValidShaderType(shaderType))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    id = glCreateShaderProgramv(shaderType, count, source);
    jassertglsucceed(context);
    jassert(id);
    return GLErrorFlags::succeed;
  }

  GLErrorFlags linkToShader(GLuint shader)
  {
    if (!id || !shader)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(isValid());
    glAttachShader(id, shader);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  GLErrorFlags unlinkFromShader(GLuint shader)
  {
    if (!id || !shader)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(isValid());
    glDetachShader(id, shader);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  static bool isValidVariableName(const GLchar* name)
    {return name && memcmp(name, "gl_", 3);}

  GLErrorFlags linkVariableNameToAttributeLocation(GLuint index, const GLchar* name)
  {
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!isValidVariableName(name))
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!context.isValidVertexAttributeIndex(index))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    glBindAttribLocation(id, index, name);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  virtual GLErrorFlags build()
  {
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(isValid());
    glLinkProgram(id);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  virtual GLErrorFlags validate()
  {
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(isValid());
    glValidateProgram(id);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  template<typename GLType>
  GLErrorFlags setUniformValue(GLint location, GLint size, GLsizei count, GLType value)
  {
    switch (size)
    {
    case 1: return GLProgramUniformRegister<GLType, 1>(*this, location, count).setValue(value);
    case 2: return GLProgramUniformRegister<GLType, 2>(*this, location, count).setValue(value);
    case 3: return GLProgramUniformRegister<GLType, 3>(*this, location, count).setValue(value);
    case 4: return GLProgramUniformRegister<GLType, 4>(*this, location, count).setValue(value);
    default: jassertfalse; return GLErrorFlags::invalidValueFlag;
    }
  }

  template<GLint numRows, GLint numColumns, typename GLType>
  GLErrorFlags setUniformMatrix(GLint location, GLsizei count, GLboolean transpose, const GLType& value)
    {return GLProgramUniformMatrixRegister<numRows, numColumns>(*this, location, count, transpose).setValue(value);}

  // properties Warning: possible OpenGL flush performance penalty.
  template<typename GLType>
  GLErrorFlags getUniformValue(GLint location, GLint size, GLsizei count, GLType value) const
  {
    switch (size)
    {
    case 1: return GLProgramUniformRegister<GLType, 1>(*this, location, count).getValue(); break;
    case 2: return GLProgramUniformRegister<GLType, 2>(*this, location, count).getValue(); break;
    case 3: return GLProgramUniformRegister<GLType, 3>(*this, location, count).getValue(); break;
    case 4: return GLProgramUniformRegister<GLType, 4>(*this, location, count).getValue(); break;
    default: jassertfalse; return GLErrorFlags::invalidValueFlag;
    }
  }

  template<GLint numRows, GLint numColumns, typename GLType>
  GLErrorFlags getUniformMatrix(GLint location, GLsizei count, GLboolean transpose, GLType& value)
    {return GLProgramUniformMatrixRegister<numRows, numColumns>(*this, location, count, transpose).getValue(value);}

  GLErrorFlags wasFlaggedForDeletion(GLboolean& flaggedForDeletion) const;
  GLErrorFlags getNumShaders(GLsizei& numShaders) const;
  GLErrorFlags getNumActiveAttributes(GLsizei& numActiveAttributes) const;
  GLErrorFlags getNumCharacteresOfLonguestActiveAttributeName(GLsizei& numCharacteresOfLonguestActiveAttributeName) const;
  GLErrorFlags getNumActiveUniforms(GLsizei& numActiveUniforms) const;
  GLErrorFlags getNumCharacteresOfLonguestActiveUniformName(GLsizei& numCharacteresOfLonguestActiveUniformName) const;
  GLErrorFlags getMaxGeometryOutputVerticesCount(GLsizei& maxGeometryOutputVerticesCount) const;
  GLErrorFlags getGeometryInputPrimitiveType(GLsizei& geometryInputPrimitiveType) const;
  GLErrorFlags getGeometryOutputPrimitiveType(GLsizei& geometryOutputPrimitiveType) const;
  virtual GLErrorFlags hasBuildSucceed(GLboolean& buildSucceed) const;
  virtual GLErrorFlags hasValidationSucceed(GLboolean& validationSucceed) const;
  virtual GLErrorFlags getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const;
  virtual GLErrorFlags getLastLog(GLsizei maxLength, GLsizei* length, GLchar* infoLog) const
  {
    if (length)
      *length = 0;
    if (maxLength < 0 || !infoLog)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    jassert(isValid());
    context.clearErrorFlags();
    glGetProgramInfoLog(id, maxLength, length, infoLog);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  // parameters (to set before linking)
  GLErrorFlags hasBinaryRetrievableHint(GLboolean& isRetrievable) const;
  GLErrorFlags setBinaryRetrievableHint(GLboolean isRetrievable);
  // for ProgramPipeline
  GLErrorFlags isSeparable(GLboolean& isSeparable) const;
  GLErrorFlags setSeparable(GLboolean isSeparable);

  // for attributes
  GLErrorFlags getAttributeVariableLocation(const GLchar* name, GLint& location)
  {
    if (!isValid())
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!isValidVariableName(name))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    context.clearErrorFlags();
    location = glGetAttribLocation(id, name);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
      {jassertfalse; return errorFlags;}
    if (location == -1)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    return errorFlags;
  }

  // for uniforms
  GLErrorFlags getUniformVariableLocation(const GLchar* name, GLint& location)
  {
    if (!isValid())
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!isValidVariableName(name))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    context.clearErrorFlags();
    location = glGetUniformLocation(id, name);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
      {jassertfalse; return errorFlags;}
    if (location == -1)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;} // compiler can remove unused uniform.
    return errorFlags;
  }

  // for subroutines
  GLErrorFlags getSubroutineUniformLocation(GLenum shaderType, const GLchar* name, GLint& location)
  {
    if (!GLEW_ARB_shader_subroutine)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;} // // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    if (!isValid())
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!context.isValidShaderType(shaderType))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    location = glGetSubroutineUniformLocation(id, shaderType, name);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
      {jassertfalse; return errorFlags;}
    if (location == -1)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;} // compiler can remove unused uniform.
    return errorFlags;
    context.clearErrorFlags();
  }

  GLErrorFlags getActiveSubRoutines(GLenum shaderType, GLint& value) const;
  GLErrorFlags getActiveSubRoutineUniforms(GLenum shaderType, GLint& value) const;         // number of subroutine uniforms
  GLErrorFlags getActiveSubRoutineUniformLocations(GLenum shaderType, GLint& value) const; // number of subroutine uniforms location (less or equal to uniforms because of arrays)
  GLErrorFlags getActiveSubRoutineMaxLength(GLenum shaderType, GLint& value) const;
  GLErrorFlags getActiveSubRoutineUniformMaxLength(GLenum shaderType, GLint& value) const;

  GLErrorFlags getSubroutineIndex(GLenum shaderType, const GLchar* name, GLuint& index)
  {
    if (!GLEW_ARB_shader_subroutine)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;} // // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    if (!isValid())
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!context.isValidShaderType(shaderType))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    index = glGetSubroutineIndex(id, shaderType, name);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasErrors())
      {jassertfalse; return errorFlags;}
    if (index == GL_INVALID_INDEX)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    return errorFlags;
    context.clearErrorFlags();
  }

  // todo glGetActiveSubroutineUniformiv glGetActiveSubroutineUniformName glGetActiveSubroutineName

protected:
  // GLObject interface
  virtual GLboolean isValidName(GLuint id) const
    {return glIsProgram(id);}
  virtual void deleteNames(GLuint id) const
    {glDeleteProgram(id);}
};

template <class GLType, GLenum propertyName>
class GLProgramProperty : public GLProperty<GLType>
{
public:
  GLProgramProperty(const GLProgramObject& programObject)
    : GLProperty<GLType>(programObject.getContext()), programObject(programObject)
    {jassert(programObject.isValid());}

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    GLint intValue = 0;
    GLProperty<GLType>::context.clearErrorFlags();
    glGetProgramiv(programObject.getId(), propertyName, &intValue);
    GLErrorFlags errorFlags = GLProperty<GLType>::context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
     {jassertfalse;}
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

private:
   const GLProgramObject& programObject;
};

// GLProgramObject
GLErrorFlags GLProgramObject::wasFlaggedForDeletion(GLboolean& flaggedForDeletion) const
  {return GLProgramProperty<GLboolean, GL_DELETE_STATUS>(*this).getValue(flaggedForDeletion);}
GLErrorFlags GLProgramObject::hasBuildSucceed(GLboolean& buildSucceed) const
  {return GLProgramProperty<GLboolean, GL_LINK_STATUS>(*this).getValue(buildSucceed);}
GLErrorFlags GLProgramObject::hasValidationSucceed(GLboolean& validationSucceed) const
  {return GLProgramProperty<GLboolean, GL_VALIDATE_STATUS>(*this).getValue(validationSucceed);}
GLErrorFlags GLProgramObject::getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const
  {return GLProgramProperty<GLsizei, GL_INFO_LOG_LENGTH>(*this).getValue(numCharacteresOfLastBuildLog);}
GLErrorFlags GLProgramObject::getNumShaders(GLsizei& numShaders) const
  {return GLProgramProperty<GLsizei, GL_ATTACHED_SHADERS>(*this).getValue(numShaders);}
GLErrorFlags GLProgramObject::getNumActiveAttributes(GLsizei& numActiveAttributes) const
  {return GLProgramProperty<GLsizei, GL_ACTIVE_ATTRIBUTES>(*this).getValue(numActiveAttributes);}
GLErrorFlags GLProgramObject::getNumCharacteresOfLonguestActiveAttributeName(GLsizei& numCharacteresOfLonguestActiveAttributeName) const
  {return GLProgramProperty<GLsizei, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH>(*this).getValue(numCharacteresOfLonguestActiveAttributeName);}
GLErrorFlags GLProgramObject::getNumActiveUniforms(GLsizei& numActiveUniforms) const
  {return GLProgramProperty<GLsizei, GL_ACTIVE_UNIFORMS>(*this).getValue(numActiveUniforms);}
GLErrorFlags GLProgramObject::getNumCharacteresOfLonguestActiveUniformName(GLsizei& numCharacteresOfLonguestActiveUniformName) const
  {return GLProgramProperty<GLsizei, GL_ACTIVE_UNIFORM_MAX_LENGTH>(*this).getValue(numCharacteresOfLonguestActiveUniformName);}
// todo GL_TRANSFORM_FEEDBACK_BUFFER_MODE GL_TRANSFORM_FEEDBACK_VARYINGS GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH
GLErrorFlags GLProgramObject::getMaxGeometryOutputVerticesCount(GLsizei& maxGeometryOutputVerticesCount) const
  {return GLProgramProperty<GLsizei, GL_GEOMETRY_VERTICES_OUT>(*this).getValue(maxGeometryOutputVerticesCount);}
GLErrorFlags GLProgramObject::getGeometryInputPrimitiveType(GLsizei& geometryInputPrimitiveType) const
  {return GLProgramProperty<GLsizei, GL_GEOMETRY_INPUT_TYPE>(*this).getValue(geometryInputPrimitiveType);}
GLErrorFlags GLProgramObject::getGeometryOutputPrimitiveType(GLsizei& geometryOutputPrimitiveType) const
  {return GLProgramProperty<GLsizei, GL_GEOMETRY_OUTPUT_TYPE>(*this).getValue(geometryOutputPrimitiveType);}
// todo manage program Binary OpenGL 4.1

template <class GLType, GLenum propertyName>
class GLProgramStageProperty : public GLProperty<GLType>
{
public:
  GLProgramStageProperty(const GLProgramObject& programObject, GLenum shaderType)
    : GLProperty<GLType>(programObject.getContext()), programObject(programObject), shaderType(shaderType)
  {
    jassert(programObject.isValid());
    jassert(GLProperty<GLType>::context.isValidShaderType(shaderType));
  }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    if (!GLEW_ARB_shader_subroutine)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;} // // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    GLint intValue = 0;
    GLProperty<GLType>::context.clearErrorFlags();
    glGetProgramStageiv(programObject.getId(), shaderType, propertyName, &intValue);
    const GLErrorFlags errorFlags = GLProperty<GLType>::context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

private:
   const GLProgramObject& programObject;
   const GLenum shaderType;
};

GLErrorFlags GLProgramObject::getActiveSubRoutines(GLenum shaderType, GLint& value) const
  {return GLProgramStageProperty<GLint, GL_ACTIVE_SUBROUTINES>(*this, shaderType).getValue(value);}
GLErrorFlags GLProgramObject::getActiveSubRoutineUniforms(GLenum shaderType, GLint& value) const
  {return GLProgramStageProperty<GLint, GL_ACTIVE_SUBROUTINE_UNIFORMS>(*this, shaderType).getValue(value);}
GLErrorFlags GLProgramObject::getActiveSubRoutineUniformLocations(GLenum shaderType, GLint& value) const
  {return GLProgramStageProperty<GLint, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS>(*this, shaderType).getValue(value);}
GLErrorFlags GLProgramObject::getActiveSubRoutineMaxLength(GLenum shaderType, GLint& value) const
  {return GLProgramStageProperty<GLint, GL_ACTIVE_SUBROUTINE_MAX_LENGTH>(*this, shaderType).getValue(value);}
GLErrorFlags GLProgramObject::getActiveSubRoutineUniformMaxLength(GLenum shaderType, GLint& value) const
  {return GLProgramStageProperty<GLint, GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH>(*this, shaderType).getValue(value);}

template <class GLProgramParameterType, GLenum parameterName>
class GLProgramParameterRegister : public GLRegister<GLProgramParameterType>
{
public:
  typedef GLProgramParameterType GLType;

  GLProgramParameterRegister(const GLProgramObject& programObject)
    : GLRegister<GLProgramParameterType>(programObject.getContext()), programObject(programObject)
    {jassert(programObject.isValid());}

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    GLint intValue = 0;
    GLRegister<GLType>::context.clearErrorFlags();
    glGetProgramiv(programObject.getId(), parameterName, &intValue);
    GLErrorFlags errorFlags = GLRegister<GLType>::context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
     {jassertfalse;}
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

  virtual GLErrorFlags setValue(const GLType& param)
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;} // // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
    if (param < 0)
		  return GLErrorFlags::invalidValueFlag;
    if (!programObject.getId())
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    glProgramParameteri(programObject.getId(), parameterName, param);
    jassertglsucceed(GLRegister<GLType>::context);
    return GLErrorFlags::succeed;
  }

private:
   const GLProgramObject& programObject;
};

GLErrorFlags GLProgramObject::hasBinaryRetrievableHint(GLboolean& isRetrievable) const
  {return GLProgramParameterRegister<GLboolean,  GL_PROGRAM_BINARY_RETRIEVABLE_HINT>(*this).getValue(isRetrievable);}
GLErrorFlags GLProgramObject::setBinaryRetrievableHint(GLboolean isRetrievable)
  {return GLProgramParameterRegister<GLboolean,  GL_PROGRAM_BINARY_RETRIEVABLE_HINT>(*this).setValue(isRetrievable);}
GLErrorFlags GLProgramObject::isSeparable(GLboolean& isSeparable) const
  {return GLProgramParameterRegister<GLboolean,  GL_PROGRAM_SEPARABLE>(*this).getValue(isSeparable);}
GLErrorFlags GLProgramObject::setSeparable(GLboolean isSeparable)
  {return GLProgramParameterRegister<GLboolean,  GL_PROGRAM_SEPARABLE>(*this).setValue(isSeparable);}

//////////////////////////////////////////////////////////////////////////////

template <class GLProgramUniformRegisterType, GLint size>
class GLProgramUniformRegister : public GLRegister<GLProgramUniformRegisterType>
{
public:
  typedef GLProgramUniformRegisterType GLType;

  GLProgramUniformRegister(const GLProgramObject& programObject, GLint location, GLsizei count)
    : GLRegister<GLProgramUniformRegisterType>(programObject.getContext())
    , programObject(programObject), location(location), count(count)
    {jassert(programObject.isValid());}

  virtual GLErrorFlags setValue(const GLType& param)
  {
    if (size < 1 || size > 4)
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    if (location == -1)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    GLuint program = programObject.getId();
    if (!program)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    GLScopedSetValue<GLenum> _(GLRegister<GLProgramUniformRegisterType>::context.getActiveProgramBind(), program);

    switch (size)
    {
    case 1: setUniform1d(location, count, param); break;
    case 2: setUniform2d(location, count, param); break;
    case 3: setUniform3d(location, count, param); break;
    case 4: setUniform4d(location, count, param); break;
    default:
      jassertfalse; // unknow uniform size type
    }

    jassertglsucceed(GLRegister<GLProgramUniformRegisterType>::context);
    return GLErrorFlags::succeed;
  }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    jassertfalse; // not yet implemented TODO
    return GLErrorFlags::invalidEnumFlag;
  }

private:
  void setUniform1d(GLint location, GLsizei count, const GLfloat* value)
    {glUniform1fv(location, count, value);}
  void setUniform2d(GLint location, GLsizei count, const GLfloat* value)
    {glUniform2fv(location, count, value);}
  void setUniform3d(GLint location, GLsizei count, const GLfloat* value)
    {glUniform3fv(location, count, value);}
  void setUniform4d(GLint location, GLsizei count, const GLfloat* value)
    {glUniform4fv(location, count, value);}

  void setUniform1d(GLint location, GLsizei count, const GLint* value)
    {glUniform1iv(location, count, value);}
  void setUniform2d(GLint location, GLsizei count, const GLint* value)
    {glUniform2iv(location, count, value);}
  void setUniform3d(GLint location, GLsizei count, const GLint* value)
    {glUniform3iv(location, count, value);}
  void setUniform4d(GLint location, GLsizei count, const GLint* value)
    {glUniform4iv(location, count, value);}

  void setUniform1d(GLint location, GLsizei count, const GLuint* value)
    {glUniform1uiv(location, count, value);}
  void setUniform2d(GLint location, GLsizei count, const GLuint* value)
    {glUniform2uiv(location, count, value);}
  void setUniform3d(GLint location, GLsizei count, const GLuint* value)
    {glUniform3uiv(location, count, value);}
  void setUniform4d(GLint location, GLsizei count, const GLuint* value)
    {glUniform4uiv(location, count, value);}

private:
   const GLProgramObject& programObject;
   const GLint location;
   const GLsizei count;
};

template <GLint numRows, GLint numColumns>
class GLProgramUniformMatrixRegister : public GLRegister<GLfloat*>
{
public:

  typedef GLfloat* GLType;

  GLProgramUniformMatrixRegister(const GLProgramObject& programObject, GLint location, GLsizei count, GLboolean transpose)
    : GLRegister<GLType>(programObject.getContext())
    , programObject(programObject), location(location), count(count), transpose(transpose)
    {jassert(programObject.isValid());}

  virtual GLErrorFlags setValue(const GLType& param)
  {
    if (!(numRows == 2 && numColumns == 2) &&
        !(numRows == 3 && numColumns == 3) &&
        !(numRows == 4 && numColumns == 4) &&
        !(numRows == 2 && numColumns == 3) &&
        !(numRows == 3 && numColumns == 2) &&
        !(numRows == 2 && numColumns == 4) &&
        !(numRows == 4 && numColumns == 2) &&
        !(numRows == 3 && numColumns == 4) &&
        !(numRows == 4 && numColumns == 3))
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    if (location == -1)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    GLuint program = programObject.getId();
    if (!program)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    GLScopedSetValue<GLenum> _(context.getActiveProgramBind(), program);

    if (numRows == 2 && numColumns == 2)
      glUniformMatrix2fv(location, count, transpose, param);
    else if (numRows == 3 && numColumns == 3)
      glUniformMatrix3fv(location, count, transpose, param);
    else if (numRows == 4 && numColumns == 4)
      glUniformMatrix4fv(location, count, transpose, param);
    else if (numRows == 2 && numColumns == 3)
      glUniformMatrix2x3fv(location, count, transpose, param);
    else if (numRows == 3 && numColumns == 2)
      glUniformMatrix3x2fv(location, count, transpose, param);
    else if (numRows == 2 && numColumns == 4)
      glUniformMatrix2x4fv(location, count, transpose, param);
    else if (numRows == 4 && numColumns == 2)
      glUniformMatrix4x2fv(location, count, transpose, param);
    else if (numRows == 3 && numColumns == 4)
      glUniformMatrix3x4fv(location, count, transpose, param);
    else if (numRows == 4 && numColumns == 3)
      glUniformMatrix4x3fv(location, count, transpose, param);
    else
      {jassertfalse;}  // unknow uniform size type

    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    jassertfalse; // not yet implemented TODO
    return GLErrorFlags::invalidEnumFlag;
  }

private:
   const GLProgramObject& programObject;
   const GLint location;
   const GLsizei count;
   const GLboolean transpose;
};

//////////////////////////////////////////////////////////////////////////////

// Predeclaration
template <class GLProgramPipelineUniformRegisterType, GLint size>
class GLProgramPipelineUniformRegister;

class GLProgramPipelineObject : public GLObject, public GLValidableInterface
{
public:
  GLProgramPipelineObject(GLContext& context)
    : GLObject(context) {}

  GLErrorFlags create()
  {
    if (!GLEW_ARB_separate_shader_objects) // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(!id); // overwritting existing: potential memory leak
    // optimizing: here we no not need to access to OpenGL errorFlags.
    glGenProgramPipelines(1, &id);
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  };

  GLErrorFlags linkToProgram(GLbitfield stages, GLuint program)
  {
    if (!GLEW_ARB_separate_shader_objects) // this is not an OpenGL3.3 feature (ARB extension included in OpenGL 4.1)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!isValidStagesBitField(stages))
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}
    if (!id || !program)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(isValid());
    glUseProgramStages(id, stages, program);
    jassertglsucceed(context); // ensure program object that was linked with its GL_PROGRAM_SEPARABLE status
    return GLErrorFlags::succeed;
  }

  virtual GLErrorFlags validate()
  {
    if (!id)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(isValid());
    glValidateProgramPipeline(id);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

  // specifies the program that Uniform* commands will update.
  // Prefere ProgramUniform* interface to reduces API overhead.
  GLErrorFlags getActiveProgram(GLuint& program) const;
  GLErrorFlags setActiveProgram(GLuint program);

  virtual GLErrorFlags hasValidationSucceed(GLboolean& validationSucceed) const;
  virtual GLErrorFlags getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const;
  virtual GLErrorFlags getLastLog(GLsizei maxLength, GLsizei* length, GLchar* infoLog) const
  {
    if (length)
      *length = 0;
    if (maxLength < 0 || !infoLog)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    jassert(isValid());
    context.clearErrorFlags();
    glGetProgramPipelineInfoLog(id, maxLength, length, infoLog);
    const GLErrorFlags errorFlags = context.popErrorFlags();
    jassert(errorFlags.hasSucceed());
    return errorFlags;
  }

  template<typename GLType>
  GLErrorFlags setProgramUniformValue(GLuint program, GLint location, GLint size, GLsizei count, GLType value)
  {
    switch (size)
    {
    case 1: return GLProgramPipelineUniformRegister<GLType, 1>(*this, program, location, count).setValue(value);
    case 2: return GLProgramPipelineUniformRegister<GLType, 2>(*this, program, location, count).setValue(value);
    case 3: return GLProgramPipelineUniformRegister<GLType, 3>(*this, program, location, count).setValue(value);
    case 4: return GLProgramPipelineUniformRegister<GLType, 4>(*this, program, location, count).setValue(value);
    default: jassertfalse; return GLErrorFlags::invalidValueFlag;
    }
  }

  static bool isValidStagesBitField(GLbitfield stages)
  {
    if (stages == GL_ALL_SHADER_BITS)
      return true;
    return (stages & ~(GL_VERTEX_SHADER_BIT |
                       GL_TESS_CONTROL_SHADER_BIT |
                       GL_TESS_EVALUATION_SHADER_BIT |
                       GL_GEOMETRY_SHADER_BIT |
                       GL_FRAGMENT_SHADER_BIT)) == 0;
  }

protected:
  // GLObject interface
  virtual GLboolean isValidName(GLuint id) const
    {return GLEW_ARB_separate_shader_objects ? glIsProgramPipeline(id) : false;}
  virtual void deleteNames(GLuint id) const
    {if (GLEW_ARB_separate_shader_objects) glDeleteProgramPipelines(1, &id);}
};

template <class GLProgramPipelinePropertyType, GLenum propertyName, GLint defaultValue = 0>
class GLProgramPipelineProperty : public GLProperty<GLProgramPipelinePropertyType>
{
public:
  typedef GLProgramPipelinePropertyType GLType;

  GLProgramPipelineProperty(const GLProgramPipelineObject& pipelineObject)
    : GLProperty<GLProgramPipelinePropertyType>(pipelineObject.getContext()), pipelineObject(pipelineObject)
    {jassert(pipelineObject.isValid());}

  virtual GLType getDefaultValue() const
    {return static_cast<GLType>(defaultValue);}

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!pipelineObject.getId())
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(pipelineObject.isValid());
    GLProperty<GLProgramPipelinePropertyType>::context.clearErrorFlags();
    GLint intValue = defaultValue;
    glGetProgramPipelineiv(pipelineObject.getId(), GL_ACTIVE_PROGRAM, &intValue);
    GLErrorFlags errorFlags = GLProperty<GLProgramPipelinePropertyType>::context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    param = static_cast<GLType>(intValue);
    return errorFlags;
  }

private:
   const GLProgramPipelineObject& pipelineObject;
};

GLErrorFlags GLProgramPipelineObject::hasValidationSucceed(GLboolean& validationSucceed) const
  {return GLProgramPipelineProperty<GLboolean, GL_VALIDATE_STATUS>(*this).getValue(validationSucceed);}
GLErrorFlags GLProgramPipelineObject::getNumCharacteresOfLastLog(GLsizei& numCharacteresOfLastBuildLog) const
  {return GLProgramPipelineProperty<GLsizei, GL_INFO_LOG_LENGTH>(*this).getValue(numCharacteresOfLastBuildLog);}

class GLProgramPipelineActiveProgramRegister : public GLRegister<GLuint>
{
public:
  GLProgramPipelineActiveProgramRegister(const GLProgramPipelineObject& pipelineObject)
    : GLRegister<GLuint>(pipelineObject.getContext()), pipelineObject(pipelineObject) {}

  virtual GLuint getDefaultValue() const
    {return 0;} // TODO check specification

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLuint& program) const
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!pipelineObject.getId())
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(pipelineObject.isValid());
    context.clearErrorFlags();
    GLint intValue = 0; // TODO check specification
    glGetProgramPipelineiv(pipelineObject.getId(), GL_ACTIVE_PROGRAM, &intValue);
    GLErrorFlags errorFlags = context.popErrorFlags();
    if (errorFlags.hasSucceed())
    {
      if (intValue < 0)
        {jassertfalse; errorFlags = GLErrorFlags::invalidValueFlag;}
    }
    else
      {jassertfalse;}
    program = static_cast<GLuint>(intValue);
    return errorFlags;
  }
  // todo manage property with glGetProgramPipelineiv and GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER,
  // GL_TESS_EVALUATION_SHADER GL_GEOMETRY_SHADER,and GL_FRAGMENT_SHADER

  virtual GLErrorFlags setValue(const GLuint& program)
  {
    if (!GLEW_ARB_separate_shader_objects)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    if (!pipelineObject.getId() || !program)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    jassert(pipelineObject.isValid());
    glActiveShaderProgram(pipelineObject.getId(), program);
    jassertglsucceed(context);
    return GLErrorFlags::succeed;
  }

private:
   const GLProgramPipelineObject& pipelineObject;
};

GLErrorFlags GLProgramPipelineObject::getActiveProgram(GLuint& program) const
  {return GLProgramPipelineActiveProgramRegister(*this).getValue(program);}
GLErrorFlags GLProgramPipelineObject::setActiveProgram(GLuint program)
  {return GLProgramPipelineActiveProgramRegister(*this).setValue(program);}


template <class GLProgramPipelineUniformRegisterType, GLint size>
class GLProgramPipelineUniformRegister : public GLRegister<GLProgramPipelineUniformRegisterType>
{
public:
  typedef GLProgramPipelineUniformRegisterType GLType;

  GLProgramPipelineUniformRegister(const GLProgramPipelineObject& pipelineObject, GLuint program, GLint location, GLsizei count)
    : GLRegister<GLProgramPipelineUniformRegisterType>(pipelineObject.getContext())
    , pipelineObject(pipelineObject), program(program), location(location), count(count)
    {jassert(pipelineObject.isValid());}

  virtual GLErrorFlags setValue(const GLType& param)
  {
    if (size < 1 || size > 4)
      {jassertfalse; return GLErrorFlags::invalidEnumFlag;}
    if (location == -1 || !program)
      {jassertfalse; return GLErrorFlags::invalidOperationFlag;}
    GLuint programPipeline = pipelineObject.getId();
    if (!programPipeline)
      {jassertfalse; return GLErrorFlags::invalidValueFlag;}

    switch (size)
    {
    case 1: setUniform1d(program, location, count, param); break;
    case 2: setUniform2d(program, location, count, param); break;
    case 3: setUniform3d(program, location, count, param); break;
    case 4: setUniform4d(program, location, count, param); break;
    default:
      jassertfalse; // unknow uniform size type
    }

    jassertglsucceed(GLRegister<GLProgramPipelineUniformRegisterType>::context);
    return GLErrorFlags::succeed;
  }

  // Warning: possible OpenGL flush performance penalty.
  virtual GLErrorFlags getValue(GLType& param) const
  {
    jassertfalse; // not yet implemented TODO
    return GLErrorFlags::invalidEnumFlag;
  }

private:
  void setUniform1d(GLuint program, GLint location, GLsizei count, const GLfloat* value)
    {glProgramUniform1fv(program, location, count, value);}
  void setUniform2d(GLuint program, GLint location, GLsizei count, const GLfloat* value)
    {glProgramUniform2fv(program, location, count, value);}
  void setUniform3d(GLuint program, GLint location, GLsizei count, const GLfloat* value)
    {glProgramUniform3fv(program, location, count, value);}
  void setUniform4d(GLuint program, GLint location, GLsizei count, const GLfloat* value)
    {glProgramUniform4fv(program, location, count, value);}

  void setUniform1d(GLuint program, GLint location, GLsizei count, const GLint* value)
    {glProgramUniform1iv(program, location, count, value);}
  void setUniform2d(GLuint program, GLint location, GLsizei count, const GLint* value)
    {glProgramUniform2iv(program, location, count, value);}
  void setUniform3d(GLuint program, GLint location, GLsizei count, const GLint* value)
    {glProgramUniform3iv(program, location, count, value);}
  void setUniform4d(GLuint program, GLint location, GLsizei count, const GLint* value)
    {glProgramUniform4iv(program, location, count, value);}

  void setUniform1d(GLuint program, GLint location, GLsizei count, const GLuint* value)
    {glProgramUniform1uiv(program, location, count, value);}
  void setUniform2d(GLuint program, GLint location, GLsizei count, const GLuint* value)
    {glProgramUniform2uiv(program, location, count, value);}
  void setUniform3d(GLuint program, GLint location, GLsizei count, const GLuint* value)
    {glProgramUniform3uiv(program, location, count, value);}
  void setUniform4d(GLuint program, GLint location, GLsizei count, const GLuint* value)
    {glProgramUniform4uiv(program, location, count, value);}

private:
   const GLProgramPipelineObject& pipelineObject;
   const GLuint program;
   const GLint location;
   const GLsizei count;
};

//////////////////////////////////////////////////////////////////////////////

// TODO: PBO, FBO with multisampling
// TODO: get uniform + set matrix uniform on program pipeline.
// TODO: compute texture size + (nvidia memory extension check)  + upload/draw timing

}; // namespace docgl

#endif // DOCGL_H_
