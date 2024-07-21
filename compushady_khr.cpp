#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/disassemble.h>
#include <spirv_msl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_glsl.hpp>
#include <sstream>

#ifdef _WIN32
#define COMPUSHADY_KHR_API __declspec(dllexport)
#else
#define COMPUSHADY_KHR_API
#endif

extern "C"
{
    COMPUSHADY_KHR_API const uint8_t *compushady_khr_glsl_to_spv(const char *glsl, const size_t glsl_size, void *(*allocator)(const size_t), size_t *spv_size)
    {
        uint8_t *spv = nullptr;
        *spv_size = 0;

        if (!allocator)
        {
            allocator = malloc;
        }

        glslang::InitializeProcess();

        {

            glslang::TProgram program;

            glslang::TShader shader(EShLanguage::EShLangCompute);

            const int glsl_size_int = static_cast<int>(glsl_size);

            shader.setStringsWithLengths(&glsl, &glsl_size_int, 1);
            shader.setEntryPoint("main");
            shader.setEnvInput(glslang::EShSource::EShSourceGlsl, EShLanguage::EShLangCompute, glslang::EShClient::EShClientVulkan, 100);
            shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EshTargetClientVersion::EShTargetVulkan_1_0);
            shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);

            if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault))
            {
                printf("AAAAAAA! %s\n", shader.getInfoLog());
            }

            program.addShader(&shader);

            printf("OK\n");

            if (!program.link(EShMsgDefault))
            {
                printf("OOPS\n");
            }

            if (!program.mapIO())
            {
                printf("OOPS2\n");
            }

            printf("OK2\n");

            printf("InfoLog: %s\n", program.getInfoLog());
            printf("InfoDebugLog: %s\n", program.getInfoDebugLog());

            // program.buildReflection(EShReflectionDefault);

            glslang::TIntermediate *intermediate = program.getIntermediate(EShLanguage::EShLangCompute);

            printf("intermediate = %p\n", intermediate);

            std::vector<uint32_t> spirv;

            spv::SpvBuildLogger logger;
            glslang::SpvOptions spv_options = {};
            spv_options.disassemble = false;
            spv_options.validate = true;
            spv_options.generateDebugInfo = true;

            printf("before spv\n");

            glslang::GlslangToSpv(*intermediate, spirv, &logger, &spv_options);

            printf("generated %d opcodes\n", spirv.size());

            *spv_size = spirv.size() * 4;
            spv = reinterpret_cast<uint8_t *>(allocator(*spv_size));

            ::memcpy(spv, spirv.data(), *spv_size);
        }

        glslang::FinalizeProcess();
        return spv;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_to_hlsl(const uint8_t *spirv, const size_t spirv_size, void *(*allocator)(const size_t), size_t *hlsl_size)
    {
        uint8_t *hlsl = nullptr;
        *hlsl_size = 0;
        std::string hlsl_code;

        if (!allocator)
        {
            allocator = malloc;
        }

        try
        {
            spirv_cross::CompilerHLSL hlsl_compiler(reinterpret_cast<const uint32_t *>(spirv), spirv_size / sizeof(uint32_t));
            spirv_cross::CompilerHLSL::Options options;
            options.shader_model = 60;
            hlsl_compiler.set_hlsl_options(options);
            hlsl_code = hlsl_compiler.compile();
        }
        catch (const std::exception &e)
        {
            printf("EXCEPTION: %s\n", e.what());
        }

        if (hlsl_code.size() > 0)
        {

            *hlsl_size = hlsl_code.size();

            hlsl = reinterpret_cast<uint8_t *>(allocator(*hlsl_size));

            ::memcpy(hlsl, hlsl_code.c_str(), *hlsl_size);
        }

        return hlsl;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_to_msl(const uint8_t *spirv, const size_t spirv_size, void *(*allocator)(const size_t), size_t *msl_size)
    {
        uint8_t *msl = nullptr;
        *msl_size = 0;
        std::string msl_code;

        if (!allocator)
        {
            allocator = malloc;
        }

        try
        {
            spirv_cross::CompilerMSL msl_compiler(reinterpret_cast<const uint32_t *>(spirv), spirv_size / sizeof(uint32_t));
            spirv_cross::CompilerMSL::Options options;
            msl_compiler.set_msl_options(options);
            msl_code = msl_compiler.compile();
        }
        catch (const std::exception &e)
        {
            printf("EXCEPTION: %s\n", e.what());
        }

        if (msl_code.size() > 0)
        {

            *msl_size = msl_code.size();

            msl = reinterpret_cast<uint8_t *>(allocator(*msl_size));

            ::memcpy(msl, msl_code.c_str(), *msl_size);
        }

        return msl;
    }

    COMPUSHADY_KHR_API const uint8_t *compushady_khr_spv_disassemble(const uint8_t *spirv, const size_t spirv_size, void *(*allocator)(const size_t), size_t *assembly_size)
    {
        uint8_t *assembly = nullptr;
        *assembly_size = 0;

        if (!allocator)
        {
            allocator = malloc;
        }

        const uint32_t *spirv_ptr = reinterpret_cast<const uint32_t *>(spirv);
        std::vector<uint32_t> spirv_vector(spirv_ptr, spirv_ptr + (spirv_size / sizeof(uint32_t)));

        std::ostringstream assembly_stream;

        spv::Disassemble(assembly_stream, spirv_vector);

        const std::string assembly_string = assembly_stream.str();
        if (assembly_string.size() > 0)
        {

            *assembly_size = assembly_string.size();

            assembly = reinterpret_cast<uint8_t *>(allocator(*assembly_size));

            ::memcpy(assembly, assembly_string.c_str(), *assembly_size);
        }

        return assembly;
    }
}