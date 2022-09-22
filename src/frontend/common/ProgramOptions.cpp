#include "ProgramOptions.h"
#include "Runtime.h"
#include "config/Build.h"

#include <CLI/CLI.hpp>

namespace IG {
static const std::map<std::string, LogLevel> LogLevelMap{ { "fatal", L_FATAL }, { "error", L_ERROR }, { "warning", L_WARNING }, { "info", L_INFO }, { "debug", L_DEBUG } };
static const std::map<std::string, SPPMode> SPPModeMap{ { "fixed", SPPMode::Fixed }, { "capped", SPPMode::Capped }, { "continous", SPPMode::Continous } };

class MyTransformer : public CLI::Validator {
public:
    using filter_fn_t = std::function<std::string(std::string)>;

    /// This allows in-place construction
    template <typename... Args>
    MyTransformer(std::initializer_list<std::pair<std::string, std::string>> values, Args&&... args)
        : MyTransformer(CLI::TransformPairs<std::string>(values), std::forward<Args>(args)...)
    {
    }

    /// direct map of std::string to std::string
    template <typename T>
    explicit MyTransformer(T&& mapping)
        : MyTransformer(std::forward<T>(mapping), nullptr)
    {
    }

    /// This checks to see if an item is in a set: pointer or copy version. You can pass in a function that will filter
    /// both sides of the comparison before computing the comparison.
    template <typename T, typename F>
    explicit MyTransformer(T mapping, F filter_function)
    {

        static_assert(CLI::detail::pair_adaptor<typename CLI::detail::element_type<T>::type>::value,
                      "mapping must produce value pairs");
        // Get the type of the contained item - requires a container have ::value_type
        // if the type does not have first_type and second_type, these are both value_type
        using element_t    = typename CLI::detail::element_type<T>::type;               // Removes (smart) pointers if needed
        using item_t       = typename CLI::detail::pair_adaptor<element_t>::first_type; // Is value_type if not a map
        using local_item_t = typename CLI::IsMemberType<item_t>::type;                  // Will convert bad types to good ones
                                                                                        // (const char * to std::string)

        // Make a local copy of the filter function, using a std::function if not one already
        std::function<local_item_t(local_item_t)> filter_fn = filter_function;

        // This is the type name for help, it will take the current version of the set contents
        desc_function_ = [mapping]() { return CLI::detail::generate_map(CLI::detail::smart_deref(mapping), true /*The only difference*/); };

        func_ = [mapping, filter_fn](std::string& input) {
            local_item_t b;
            if (!CLI::detail::lexical_cast(input, b)) {
                return std::string();
                // there is no possible way we can match anything in the mapping if we can't convert so just return
            }
            if (filter_fn) {
                b = filter_fn(b);
            }
            auto res = CLI::detail::search(mapping, b, filter_fn);
            if (res.first) {
                input = CLI::detail::value_string(CLI::detail::pair_adaptor<element_t>::second(*res.second));
            }
            return std::string{};
        };
    }

    /// You can pass in as many filter functions as you like, they nest
    template <typename T, typename... Args>
    MyTransformer(T&& mapping, filter_fn_t filter_fn_1, filter_fn_t filter_fn_2, Args&&... other)
        : MyTransformer(
            std::forward<T>(mapping),
            [filter_fn_1, filter_fn_2](std::string a) { return filter_fn_2(filter_fn_1(a)); },
            other...)
    {
    }
};

ProgramOptions::ProgramOptions(int argc, char** argv, ApplicationType type, const std::string& desc)
{
    bool useCPU     = false;
    bool useGPU     = false;
    int threadCount = 0;
    int vectorWidth = 0;
    int device      = 0;

    Type = type;

    CLI::App app{ desc, argc >= 1 ? argv[0] : "unknown" };

    app.positionals_at_end(false);
    app.set_version_flag("--version", Build::getBuildString());
    app.set_help_flag("-h,--help", "Shows help message and exit");

    app.add_option("scene", InputScene, "Scene file to load. Can be a Ignis scene file or a glTF file.")->required()->check(CLI::ExistingFile);
    app.add_flag("-q,--quiet", Quiet, "Do not print messages into console");
    app.add_flag_callback(
        "-v,--verbose", [&]() { VerbosityLevel = L_DEBUG; }, "Set the verbosity level to 'debug'. Shortcut for --log-level debug");

    app.add_option("--log-level", VerbosityLevel, "Set the verbosity level")->transform(MyTransformer(LogLevelMap, CLI::ignore_case));

    app.add_flag("--no-color", NoColor, "Do not use decorations to make console output better");

    if (type == ApplicationType::CLI)
        app.add_flag("--no-progress", NoProgress, "Do not show progress information");

    if (type != ApplicationType::Trace) {
        const auto width  = app.add_option("--width", Width, "Set the viewport horizontal dimension (in pixels)");
        const auto height = app.add_option("--height", Height, "Set the viewport vertical dimension (in pixels)");

        width->needs(height);
        height->needs(width);

        app.add_option("--eye", Eye, "Set the position of the camera");
        app.add_option("--dir", Dir, "Set the direction vector of the camera");
        app.add_option("--up", Up, "Set the up vector of the camera");

        app.add_option("--camera", CameraType, "Override camera type")->check(CLI::IsMember(Runtime::getAvailableCameraTypes(), CLI::ignore_case));
    }

    app.add_option("--technique", TechniqueType, "Override technique type")->check(CLI::IsMember(Runtime::getAvailableTechniqueTypes(), CLI::ignore_case));
    app.add_flag_callback(
        "--debug", [&]() { TechniqueType = "debug"; }, "Same as --technique debug");

    app.add_flag("--cpu", useCPU, "Use CPU as target only");
    app.add_flag("--gpu", useGPU, "Use GPU as target only");
    app.add_option("--gpu-device", device, "Pick GPU device to use on the selected platform")->default_val(0);
    app.add_option("--cpu-threads", threadCount, "Number of threads used on a CPU target. Set to 0 to detect automatically")->default_val(threadCount);
    app.add_option("--cpu-vectorwidth", vectorWidth, "Number of vector lanes used on a CPU target. Set to 0 to detect automatically")->default_val(vectorWidth);

    app.add_option("--spp", SPP, "Enables benchmarking mode and sets the number of iterations based on the given spp");
    app.add_option("--spi", SPI, "Number of samples per iteration. This is only considered a hint for the underlying technique");
    if (type == ApplicationType::View)
        app.add_option("--spp-mode", SPPMode, "Sets the current spp mode")->transform(MyTransformer(SPPModeMap, CLI::ignore_case))->default_str("fixed");

    app.add_flag("--stats", AcquireStats, "Acquire useful stats alongside rendering. Will be dumped at the end of the rendering session");
    app.add_flag("--stats-full", AcquireFullStats, "Acquire all stats alongside rendering. Will be dumped at the end of the rendering session");

    app.add_flag("--dump-shader", DumpShader, "Dump produced shaders to files in the current working directory");
    app.add_flag("--dump-shader-full", DumpFullShader, "Dump produced shaders with standard library to files in the current working directory");

    app.add_option("--script-dir", ScriptDir, "Override internal script standard library by '.art' files from the given directory");

    app.add_flag("--add-env-light", AddExtraEnvLight, "Add additional constant environment light. This is automatically done for glTF scenes without any lights");
    app.add_flag("--force-specialization", ForceSpecialization, "Enforce specialization for parameters in shading tree. This will increase compile time drastically for potential runtime optimization");

    if (type != ApplicationType::Trace) {
        if (type == ApplicationType::CLI) {
            // Focus on quality
            DenoiserFollowSpecular     = true;
            DenoiserOnlyFirstIteration = false;
        } else {
            // Focus on interactivity
            DenoiserFollowSpecular     = false;
            DenoiserOnlyFirstIteration = true;
        }

        app.add_flag("--denoise", Denoise, "Apply denoiser if available");
        app.add_flag("--denoiser-follow-specular,!--denoiser-skip-specular", DenoiserFollowSpecular, "Follow specular paths or terminate at them");
        app.add_flag("--denoiser-aov-first-iteration,!--denoiser-aov-every-iteration", DenoiserOnlyFirstIteration, "Acquire scene normal, albedo and depth information every iteration or only at the first");
    }

    if (type == ApplicationType::Trace) {
        app.add_option("-i,--input", InputRay, "Read list of rays from file instead of the standard input");
        app.add_option("-o,--output", Output, "Write radiance for each ray into file instead of standard output");
    } else {
        app.add_option("-o,--output", Output, "Writes the output image to a file");
    }

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        app.exit(e);
        ShouldExit = true;
        return;
    }

    if (useGPU)
        Target = IG::Target::pickGPU(device);
    else if (useCPU)
        Target = IG::Target::pickCPU();
    else
        Target = IG::Target::pickBest();

    if (device >= 0)
        Target.setDevice((size_t)device);

    if (threadCount >= 0)
        Target.setThreadCount((size_t)threadCount);

    if (vectorWidth >= 4) {
        // TODO: Make sure it is greater 4 and power of 2
        Target.setVectorWidth((size_t)vectorWidth);
    }
}

void ProgramOptions::populate(RuntimeOptions& options) const
{
    IG_LOGGER.setQuiet(Quiet);
    IG_LOGGER.setVerbosity(VerbosityLevel);
    IG_LOGGER.enableAnsiTerminal(!NoColor);

    options.IsTracer      = Type == ApplicationType::Trace;
    options.IsInteractive = Type == ApplicationType::View;

    options.Target         = Target;
    options.AcquireStats   = AcquireStats || AcquireFullStats;
    options.DumpShader     = DumpShader;
    options.DumpShaderFull = DumpFullShader;
    options.SPI            = SPI.value_or(0);

    options.OverrideTechnique = TechniqueType;
    options.OverrideCamera    = CameraType;
    if (Width.has_value() && Height.has_value())
        options.OverrideFilmSize = { Width.value(), Height.value() };

    options.AddExtraEnvLight    = AddExtraEnvLight;
    options.ForceSpecialization = ForceSpecialization;

    options.Denoiser.Enabled            = Denoise;
    options.Denoiser.FollowSpecular     = DenoiserFollowSpecular;
    options.Denoiser.OnlyFirstIteration = DenoiserOnlyFirstIteration;

    options.ScriptDir = ScriptDir;
}

} // namespace IG