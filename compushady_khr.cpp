#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.h>
#include <spirv_msl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_glsl.hpp>

#ifdef _WIN32
#define COMPUSHADY_KHR_API __declspec(dllexport)
#else
#define COMPUSHADY_KHR_API
#endif

extern "C"
{
    COMPUSHADY_KHR_API void *compushady_khr_malloc(const size_t size)
    {
        return malloc(size);
    }

    COMPUSHADY_KHR_API void compushady_khr_free(void *ptr)
    {
        free(ptr);
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_glsl_to_spv(const char *glsl, const size_t glsl_size, const char *shader_model, const char* entry_point, const uint32_t flags, size_t *spv_size, char **error_ptr, size_t *error_len, void *(*allocator)(const size_t))
    {
        uint8_t *spv = nullptr;
        *spv_size = 0;
        *error_ptr = nullptr;
        *error_len = 0;

        if (!allocator)
        {
            allocator = compushady_khr_malloc;
        }

        glslang::InitializeProcess();

        {
            EShLanguage language = EShLanguage::EShLangCompute;
            if (shader_model && ::strlen(shader_model) >= 2)
            {
                if (shader_model[0] == 'c' && shader_model[1] == 's')
                {
                    language = EShLanguage::EShLangCompute;
                }
                else if (shader_model[0] == 'v' && shader_model[1] == 's')
                {
                    language = EShLanguage::EShLangVertex;
                }
                else if (shader_model[0] == 'p' && shader_model[1] == 's')
                {
                    language = EShLanguage::EShLangFragment;
                }
                else if (shader_model[0] == 'm' && shader_model[1] == 's')
                {
                    language = EShLanguage::EShLangMesh;
                }
            }

            glslang::TProgram program;

            glslang::TShader shader(language);

            const int glsl_size_int = static_cast<int>(glsl_size);

            shader.setStringsWithLengths(&glsl, &glsl_size_int, 1);
            shader.setEntryPoint(entry_point);
            shader.setEnvInput(glslang::EShSource::EShSourceGlsl, language, glslang::EShClient::EShClientVulkan, 100);
            shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EshTargetClientVersion::EShTargetVulkan_1_0);
            shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);

            if (flags & 1)
            {
                shader.setShiftBinding(glslang::TResourceType::EResUbo, 0);
                shader.setShiftBinding(glslang::TResourceType::EResImage, 1024);
                shader.setShiftBinding(glslang::TResourceType::EResTexture, 1024);
                shader.setShiftBinding(glslang::TResourceType::EResUav, 2048);
                shader.setShiftBinding(glslang::TResourceType::EResSsbo, 2048);
                shader.setShiftBinding(glslang::TResourceType::EResSampler, 3072);
            }

            if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault))
            {
                const char *info_log = shader.getInfoLog();
                *error_len = ::strlen(info_log);
                *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
                ::memcpy(*error_ptr, info_log, *error_len);
                glslang::FinalizeProcess();
                return nullptr;
            }

            program.addShader(&shader);

            if (!program.link(EShMsgDefault))
            {
                const char *info_log = program.getInfoLog();
                *error_len = ::strlen(info_log);
                *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
                ::memcpy(*error_ptr, info_log, *error_len);
                glslang::FinalizeProcess();
                return nullptr;
            }

            if (!program.mapIO())
            {
                const char *info_log = program.getInfoLog();
                *error_len = ::strlen(info_log);
                *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
                ::memcpy(*error_ptr, info_log, *error_len);
                glslang::FinalizeProcess();
                return nullptr;
            }

            glslang::TIntermediate *intermediate = program.getIntermediate(language);

            std::vector<uint32_t> spirv;

            spv::SpvBuildLogger logger;
            glslang::SpvOptions spv_options = {};
            spv_options.disassemble = false;
            spv_options.validate = true;
            spv_options.generateDebugInfo = true;
            spv_options.disableOptimizer = (flags & 2) != 0;

            glslang::GlslangToSpv(*intermediate, spirv, &logger, &spv_options);

            *spv_size = spirv.size() * 4;
            spv = reinterpret_cast<uint8_t *>(allocator(*spv_size));

            ::memcpy(spv, spirv.data(), *spv_size);
        }

        glslang::FinalizeProcess();
        return spv;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_to_hlsl(const uint8_t *spirv, const size_t spirv_size, const uint32_t flags, size_t *hlsl_size, char **entry_point_ptr, size_t *entry_point_len, char **error_ptr, size_t *error_len, void *(*allocator)(const size_t))
    {
        uint8_t *hlsl = nullptr;
        *hlsl_size = 0;
        *entry_point_len = 0;
        std::string hlsl_code;
        std::string entry_point;

        if (!allocator)
        {
            allocator = compushady_khr_malloc;
        }

        try
        {
            spirv_cross::CompilerHLSL hlsl_compiler(reinterpret_cast<const uint32_t *>(spirv), spirv_size / sizeof(uint32_t));
            spirv_cross::CompilerHLSL::Options options;
            options.shader_model = 60;
            hlsl_compiler.set_resource_binding_flags(spirv_cross::HLSL_BINDING_AUTO_ALL);
            hlsl_compiler.set_hlsl_options(options);
            hlsl_code = hlsl_compiler.compile();
            for(const spirv_cross::EntryPoint& entry_point_item  : hlsl_compiler.get_entry_points_and_stages())
            {
                if (entry_point_item.execution_model == hlsl_compiler.get_execution_model())
                {
                    entry_point = entry_point_item.name;
                }
            }
        }
        catch (const std::exception &e)
        {
            const char *exception_message = e.what();
            *error_len = ::strlen(exception_message);
            *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
            ::memcpy(*error_ptr, exception_message, *error_len);
            return NULL;
        }

        if (hlsl_code.size() > 0)
        {
            
            if (entry_point.size() > 0)
            {
                *entry_point_len = entry_point.size();
                *entry_point_ptr = reinterpret_cast<char *>(allocator(*entry_point_len));
                ::memcpy(*entry_point_ptr, entry_point.c_str(), *entry_point_len);
            }

            *hlsl_size = hlsl_code.size();

            hlsl = reinterpret_cast<uint8_t *>(allocator(*hlsl_size));

            ::memcpy(hlsl, hlsl_code.c_str(), *hlsl_size);
        }

        return hlsl;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_to_glsl(const uint8_t *spirv, const size_t spirv_size, const uint32_t flags, size_t *glsl_size, char **error_ptr, size_t *error_len, void *(*allocator)(const size_t))
    {
        uint8_t *glsl = nullptr;
        *glsl_size = 0;
        std::string glsl_code;

        if (!allocator)
        {
            allocator = compushady_khr_malloc;
        }

        try
        {
            spirv_cross::CompilerGLSL glsl_compiler(reinterpret_cast<const uint32_t *>(spirv), spirv_size / sizeof(uint32_t));
            spirv_cross::CompilerGLSL::Options options;

            glsl_compiler.set_common_options(options);
            glsl_code = glsl_compiler.compile();
        }
        catch (const std::exception &e)
        {
            const char *exception_message = e.what();
            *error_len = ::strlen(exception_message);
            *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
            ::memcpy(*error_ptr, exception_message, *error_len);
            return NULL;
        }

        if (glsl_code.size() > 0)
        {

            *glsl_size = glsl_code.size();

            glsl = reinterpret_cast<uint8_t *>(allocator(*glsl_size));

            ::memcpy(glsl, glsl_code.c_str(), *glsl_size);
        }

        return glsl;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_to_msl(const uint8_t *spirv, const size_t spirv_size, const uint32_t flags, size_t *msl_size, uint32_t *x, uint32_t *y, uint32_t *z, char **error_ptr, size_t *error_len, void *(*allocator)(const size_t))
    {
        uint8_t *msl = nullptr;
        *msl_size = 0;
        std::string msl_code;

        if (!allocator)
        {
            allocator = compushady_khr_malloc;
        }

        try
        {
            spirv_cross::CompilerMSL msl_compiler(reinterpret_cast<const uint32_t *>(spirv), spirv_size / sizeof(uint32_t));
            *x = msl_compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
            *y = msl_compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
            *z = msl_compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
            spirv_cross::CompilerMSL::Options options;
            options.set_msl_version(2, 3);
            msl_compiler.set_msl_options(options);
            msl_code = msl_compiler.compile();
        }
        catch (const std::exception &e)
        {
            const char *exception_message = e.what();
            *error_len = ::strlen(exception_message);
            *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
            ::memcpy(*error_ptr, exception_message, *error_len);
            return NULL;
        }

        if (msl_code.size() > 0)
        {

            *msl_size = msl_code.size();

            msl = reinterpret_cast<uint8_t *>(allocator(*msl_size));

            ::memcpy(msl, msl_code.c_str(), *msl_size);
        }

        return msl;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_disassemble(const uint8_t *spirv, const size_t spirv_size, const uint32_t flags, size_t *assembly_size, char **error_ptr, size_t *error_len, void *(*allocator)(const size_t))
    {
        uint8_t *assembly = nullptr;
        *assembly_size = 0;

        if (!allocator)
        {
            allocator = compushady_khr_malloc;
        }

        const uint32_t *spirv_ptr = reinterpret_cast<const uint32_t *>(spirv);

        spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_5);

        uint32_t options = SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES;

        spv_text text = nullptr;
        spv_diagnostic diagnostic = nullptr;

        spv_result_t error = spvBinaryToText(context, spirv_ptr, spirv_size / 4, options,
                                             &text, &diagnostic);
        spvContextDestroy(context);

        if (error)
        {
            const char *exception_message = diagnostic->error;
            *error_len = ::strlen(exception_message);
            *error_ptr = reinterpret_cast<char *>(allocator(*error_len));
            ::memcpy(*error_ptr, exception_message, *error_len);
            spvDiagnosticDestroy(diagnostic);
            return NULL;
        }

        *assembly_size = text->length;

        assembly = reinterpret_cast<uint8_t *>(allocator(*assembly_size));

        ::memcpy(assembly, text->str, *assembly_size);

        spvTextDestroy(text);

        return assembly;
    }
}