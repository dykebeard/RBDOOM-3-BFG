/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"



idRenderProgManager renderProgManager;

/*
================================================================================================
idRenderProgManager::idRenderProgManager()
================================================================================================
*/
idRenderProgManager::idRenderProgManager()
{
}

/*
================================================================================================
idRenderProgManager::~idRenderProgManager()
================================================================================================
*/
idRenderProgManager::~idRenderProgManager()
{
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const idCmdArgs& args )
{
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders();
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init()
{
	common->Printf( "----- Initializing Render Shaders -----\n" );
	
	
	for( int i = 0; i < MAX_BUILTINS; i++ )
	{
		builtinShaders[i] = -1;
	}
	
	// RB: added checks for GPU skinning
	struct builtinShaders_t
	{
		int index;
		const char* name;
		bool requireGPUSkinningSupport;
	} builtins[] =
	{
		{ BUILTIN_GUI, "gui.vfp", false },
		{ BUILTIN_COLOR, "color.vfp", false },
//		{ BUILTIN_SIMPLESHADE, "simpleshade.vfp", false },
		{ BUILTIN_TEXTURED, "texture.vfp", false },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color.vfp", false },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned.vfp", true },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen.vfp", false },
		{ BUILTIN_INTERACTION, "interaction.vfp", false },
		{ BUILTIN_INTERACTION_SKINNED, "interaction_skinned.vfp", true },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient.vfp", false },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned.vfp", true },
		{ BUILTIN_ENVIRONMENT, "environment.vfp", false },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned.vfp", true },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp", false },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyEnvironment_skinned.vfp", true },
		
		{ BUILTIN_DEPTH, "depth.vfp", false },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned.vfp", true },
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug.vfp", false },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned.vfp", true },
		
		{ BUILTIN_BLENDLIGHT, "blendlight.vfp", false },
		{ BUILTIN_FOG, "fog.vfp", false },
		{ BUILTIN_FOG_SKINNED, "fog_skinned.vfp", true },
		{ BUILTIN_SKYBOX, "skybox.vfp", false },
		{ BUILTIN_WOBBLESKY, "wobblesky.vfp", false },
		{ BUILTIN_POSTPROCESS, "postprocess.vfp", false },
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost.vfp", false },
		{ BUILTIN_STEREO_WARP, "stereoWarp.vfp", false },
//		{ BUILTIN_ZCULL_RECONSTRUCT, "zcullReconstruct.vfp", false },
		{ BUILTIN_BINK, "bink.vfp", false },
		{ BUILTIN_BINK_GUI, "bink_gui.vfp", false },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace.vfp", false },
//		{ BUILTIN_MOTION_BLUR, "motionBlur.vfp", false },
		{ BUILTIN_SHADOW, "shadow.vfp", false },
		{ BUILTIN_SHADOW_SKINNED, "shadow_skinned.vfp", true },
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );
	vertexShaders.SetNum( numBuiltins );
	fragmentShaders.SetNum( numBuiltins );
	glslPrograms.SetNum( numBuiltins );
	
	for( int i = 0; i < numBuiltins; i++ )
	{
		vertexShaders[i].name = builtins[i].name;
		fragmentShaders[i].name = builtins[i].name;
		builtinShaders[builtins[i].index] = i;
		
		if( builtins[i].requireGPUSkinningSupport && !glConfig.gpuSkinningAvailable )
		{
			// RB: don't try to load shaders that would break the GLSL compiler in the OpenGL driver
			continue;
		}
		
		LoadVertexShader( i );
		LoadFragmentShader( i );
		LoadGLSLProgram( i, i, i );
	}
	
	// special case handling for fastZ shaders
	/*
	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL32_CORE_PROFILE:
		case GLDRV_OPENGL_ES2:
		case GLDRV_OPENGL_ES3:
		case GLDRV_OPENGL_MESA:
		{
			builtinShaders[BUILTIN_SHADOW] = FindVertexShader( "shadow.vp" );
			int shadowFragmentShaderIndex = FindFragmentShader( "shadow.fp" );
			FindGLSLProgram( "shadow.vp", builtinShaders[BUILTIN_SHADOW], shadowFragmentShaderIndex );
	
			if( glConfig.gpuSkinningAvailable )
			{
				builtinShaders[BUILTIN_SHADOW_SKINNED] = FindVertexShader( "shadow_skinned.vp" );
				int shadowFragmentShaderIndex = FindFragmentShader( "shadow_skinned.fp" );
				FindGLSLProgram( "shadow_skinned.vp", builtinShaders[BUILTIN_SHADOW_SKINNED], shadowFragmentShaderIndex );
				break;
			}
		}
	
		default:
		{
			// fast path on PC
			builtinShaders[BUILTIN_SHADOW] = FindVertexShader( "shadow.vp" );
			FindGLSLProgram( "shadow.vp", builtinShaders[BUILTIN_SHADOW], -1 );
	
			if( glConfig.gpuSkinningAvailable )
			{
				builtinShaders[BUILTIN_SHADOW_SKINNED] = FindVertexShader( "shadow_skinned.vp" );
				FindGLSLProgram( "shadow_skinned.vp", builtinShaders[BUILTIN_SHADOW_SKINNED], -1 );
			}
		}
	}
	*/
	
	glslUniforms.SetNum( RENDERPARM_USER + MAX_GLSL_USER_PARMS, vec4_zero );
	
	if( glConfig.gpuSkinningAvailable )
	{
		vertexShaders[builtinShaders[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_ENVIRONMENT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_BUMPY_ENVIRONMENT_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_DEPTH_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_SHADOW_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_SHADOW_DEBUG_SKINNED]].usesJoints = true;
		vertexShaders[builtinShaders[BUILTIN_FOG_SKINNED]].usesJoints = true;
	}
	
	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders()
{
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		LoadVertexShader( i );
	}
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		LoadFragmentShader( i );
	}
	
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		LoadGLSLProgram( i, glslPrograms[i].vertexShaderIndex, glslPrograms[i].fragmentShaderIndex );
	}
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		if( vertexShaders[i].progId != INVALID_PROGID )
		{
			qglDeleteShader( vertexShaders[i].progId );
			vertexShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		if( fragmentShaders[i].progId != INVALID_PROGID )
		{
			qglDeleteShader( fragmentShaders[i].progId );
			fragmentShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( glslPrograms[i].progId != INVALID_PROGID )
		{
			qglDeleteProgram( glslPrograms[i].progId );
			glslPrograms[i].progId = INVALID_PROGID;
		}
	}
}

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	KillAllShaders();
}

/*
================================================================================================
idRenderProgManager::FindVertexShader
================================================================================================
*/
int idRenderProgManager::FindVertexShader( const char* name )
{
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		if( vertexShaders[i].name.Icmp( name ) == 0 )
		{
			LoadVertexShader( i );
			return i;
		}
	}
	vertexShader_t shader;
	shader.name = name;
	int index = vertexShaders.Append( shader );
	LoadVertexShader( index );
	currentVertexShader = index;
	
	// RB: removed idStr::Icmp( name, "heatHaze.vfp" ) == 0  hack
	// this requires r_useUniformArrays 1
	for( int i = 0; i < vertexShaders[index].uniforms.Num(); i++ )
	{
		if( vertexShaders[index].uniforms[i] == RENDERPARM_ENABLE_SKINNING )
		{
			vertexShaders[index].usesJoints = true;
			vertexShaders[index].optionalSkinning = true;
		}
	}
	// RB end
	
	return index;
}

/*
================================================================================================
idRenderProgManager::FindFragmentShader
================================================================================================
*/
int idRenderProgManager::FindFragmentShader( const char* name )
{
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		if( fragmentShaders[i].name.Icmp( name ) == 0 )
		{
			LoadFragmentShader( i );
			return i;
		}
	}
	fragmentShader_t shader;
	shader.name = name;
	int index = fragmentShaders.Append( shader );
	LoadFragmentShader( index );
	currentFragmentShader = index;
	return index;
}




/*
================================================================================================
idRenderProgManager::LoadVertexShader
================================================================================================
*/
void idRenderProgManager::LoadVertexShader( int index )
{
	if( vertexShaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}
	vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].uniforms );
}

/*
================================================================================================
idRenderProgManager::LoadFragmentShader
================================================================================================
*/
void idRenderProgManager::LoadFragmentShader( int index )
{
	if( fragmentShaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}
	fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].uniforms );
}

/*
================================================================================================
idRenderProgManager::BindShader
================================================================================================
*/
// RB begin
void idRenderProgManager::BindShader( int progIndex, int vIndex, int fIndex, bool builtin )
{
	if( currentVertexShader == vIndex && currentFragmentShader == fIndex )
	{
		return;
	}
	
	if( builtin )
	{
		currentVertexShader = vIndex;
		currentFragmentShader = fIndex;
		
		// vIndex denotes the GLSL program
		if( vIndex >= 0 && vIndex < glslPrograms.Num() )
		{
			currentRenderProgram = vIndex;
			RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[vIndex].name.c_str() );
			qglUseProgram( glslPrograms[vIndex].progId );
		}
	}
	else
	{
		if( progIndex == -1 )
		{
			// RB: FIXME linear search
			for( int i = 0; i < glslPrograms.Num(); ++i )
			{
				if( ( glslPrograms[i].vertexShaderIndex == vIndex ) && ( glslPrograms[i].fragmentShaderIndex == fIndex ) )
				{
					progIndex = i;
					break;
				}
			}
		}
		
		currentVertexShader = vIndex;
		currentFragmentShader = fIndex;
		
		if( progIndex >= 0 && progIndex < glslPrograms.Num() )
		{
			currentRenderProgram = progIndex;
			RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[progIndex].name.c_str() );
			qglUseProgram( glslPrograms[progIndex].progId );
		}
	}
}
// RB end

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	currentVertexShader = -1;
	currentFragmentShader = -1;
	
	qglUseProgram( 0 );
}

// RB begin
bool idRenderProgManager::IsShaderBound() const
{
	return ( currentVertexShader != -1 );
}
// RB end

/*
================================================================================================
idRenderProgManager::SetRenderParms
================================================================================================
*/
void idRenderProgManager::SetRenderParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		SetRenderParm( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* value )
{
	SetUniformValue( rp, value );
}

