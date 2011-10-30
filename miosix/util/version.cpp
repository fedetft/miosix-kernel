
namespace miosix {

#ifdef __GNUC__
#define tts(x) #x
#define ts(x) tts(x)
#define CV ", gcc "ts(__GNUC__)"."ts(__GNUC_MINOR__)"."ts(__GNUC_PATCHLEVEL__)
#define AU __attribute__((used))
#else
#define CV
#define AU
#endif

const char AU ver[]="Miosix v1.59 (" _MIOSIX ", " __DATE__ " " __TIME__ CV ")";

const char *getMiosixVersion()
{
    return ver;
}

} //namespace miosix
