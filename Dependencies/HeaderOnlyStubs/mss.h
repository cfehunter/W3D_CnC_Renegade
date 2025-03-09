#pragma once

#ifndef _MILES_STUB_HO_INCLUDED_
#define _MILES_STUB_HO_INCLUDED_

// Copied from omniblade's miles stub, but functions are implemented in the header so it's platform agnostic and entirely nulled out

#include <cstdlib>
#include <Windows.h>
#include <mmsys.h>


/*
 * These will have been structs in the real SDK headers, but are only accessed through pointers
 * so we don't care unless access to the internals is required.
 */
typedef struct h3DPOBJECT
{
	unsigned int junk;
} h3DPOBJECT;
typedef h3DPOBJECT* H3DPOBJECT;
typedef H3DPOBJECT H3DSAMPLE;
typedef struct _SAMPLE* HSAMPLE;
typedef struct _STREAM* HSTREAM;
typedef struct _DIG_DRIVER
{
	char pad[168];
	int emulated_ds; // We only care about this particular member for our purposes.
} DIG_DRIVER;
typedef struct _DIG_DRIVER* HDIGDRIVER;
typedef struct _AUDIO* HAUDIO;
typedef struct _HMDIDRIVER* HMDIDRIVER;
typedef struct _HDLSDEVICE* HDLSDEVICE;
typedef unsigned long HPROVIDER;
typedef int HTIMER;
typedef unsigned int HPROENUM;
typedef int M3DRESULT;

typedef void* AILLPDIRECTSOUND;
typedef void* AILLPDIRECTSOUNDBUFFER;

typedef struct _AILSOUNDINFO
{
	int format;
	const void* data_ptr;
	unsigned int data_len;
	unsigned int rate;
	int bits;
	int channels;
	unsigned int samples;
	unsigned int block_size;
	const void* initial_ptr;
} AILSOUNDINFO;

typedef enum
{
	DP_ASI_DECODER = 0, // Must be "ASI codec stream" provider
	DP_FILTER, // Must be "MSS pipeline filter" provider
	DP_MERGE, // Must be "MSS mixer" provider
	N_SAMPLE_STAGES, // Placeholder for end of list (= # of valid stages)
	SAMPLE_ALL_STAGES // Used to signify all pipeline stages, for shutdown
} SAMPLESTAGE;

#define AILCALLBACK __stdcall

typedef unsigned long U32;
typedef long S32;
typedef float F32;

#define AIL_set_3D_object_user_data AIL_set_3D_user_data
#define AIL_3D_object_user_data AIL_3D_user_data
#define AIL_3D_open_listener AIL_open_3D_listener

/*
 * Various callback typedefs.
 */
typedef unsigned long(__stdcall* AIL_file_open_callback)(const char*, uintptr_t*);
typedef void(__stdcall* AIL_file_close_callback)(uintptr_t);
typedef long(__stdcall* AIL_file_seek_callback)(uintptr_t, long, unsigned long);
typedef unsigned long(__stdcall* AIL_file_read_callback)(uintptr_t, void*, unsigned long);
typedef void(__stdcall* AIL_stream_callback)(HSTREAM);
typedef void(__stdcall* AIL_3dsample_callback)(H3DPOBJECT);
typedef void(__stdcall* AIL_sample_callback)(HSAMPLE);

#if !defined BUILD_STUBS && defined _WIN32
#define __declspec(dllimport)
#else
#define IMPORTS
#endif

#define DIG_USE_WAVEOUT 15
#define AIL_LOCK_PROTECTION 18
#define WAVE_FORMAT_IMA_ADPCM 0x11
#define ENVIRONMENT_GENERIC 0
#define HPROENUM_FIRST 0
#define AIL_NO_ERROR 0
#define AIL_FILE_SEEK_BEGIN 0
#define AIL_FILE_SEEK_CURRENT 1
#define AIL_FILE_SEEK_END 2
#define AIL_3D_2_SPEAKER 0
#define AIL_3D_HEADPHONE 1
#define AIL_3D_SURROUND 2
#define AIL_3D_4_SPEAKER 3
#define AIL_3D_51_SPEAKER 4
#define AIL_3D_71_SPEAKER 5
#define M3D_NOERR 0

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

inline long AIL_3D_sample_volume(H3DSAMPLE sample) { return 0; }
inline void AIL_set_3D_sample_volume(H3DSAMPLE sample, long volume) {}
inline void AIL_end_3D_sample(H3DSAMPLE sample) {}
inline void AIL_resume_3D_sample(H3DSAMPLE sample) {}
inline void AIL_stop_3D_sample(H3DSAMPLE sample) {}
inline void AIL_start_3D_sample(H3DSAMPLE sample) {}
inline int AIL_3D_sample_loop_count(H3DSAMPLE sample) { return 0; }
inline void AIL_set_3D_sample_offset(H3DSAMPLE sample, int offset) {}
inline int AIL_3D_sample_length(H3DSAMPLE sample) { return 0; }
inline int AIL_3D_sample_offset(H3DSAMPLE sample) { return 0; }
inline int AIL_3D_sample_playback_rate(H3DSAMPLE sample) { return 0; }
inline void AIL_set_3D_sample_playback_rate(H3DSAMPLE sample, int playback_rate) {}
inline int AIL_set_3D_sample_file(H3DSAMPLE sample, const void* file_image) { return 0; }
inline HPROVIDER AIL_set_sample_processor(HSAMPLE sample, SAMPLESTAGE pipeline_stage, HPROVIDER provider) { return 0; }
inline void AIL_set_filter_sample_preference(HSAMPLE sample, const char* name, const void* val) { }
inline void AIL_release_sample_handle(HSAMPLE sample) {}
inline void AIL_close_3D_provider(HPROVIDER lib) {}
inline int AIL_set_preference(unsigned int number, int value) { return 0; }
inline int AIL_waveOutOpen(HDIGDRIVER* driver, LPHWAVEOUT* waveout, int id, LPWAVEFORMAT format) { return 0; }
inline void AIL_waveOutClose(HDIGDRIVER driver) {}
inline void AIL_set_3D_sample_loop_count(H3DSAMPLE sample, int count) {}
inline void AIL_set_stream_playback_rate(HSTREAM stream, int rate) {}
inline int AIL_stream_playback_rate(HSTREAM stream) { return 0; }
inline void AIL_stream_ms_position(HSTREAM sample, S32* total_milliseconds, S32* current_milliseconds) {}
inline void AIL_set_stream_ms_position(HSTREAM stream, int pos) {}
inline int AIL_stream_loop_count(HSTREAM stream) { return 0; }
inline void AIL_set_stream_loop_block(HSTREAM stream, int loop_start, int loop_end) {}
inline void AIL_set_stream_loop_count(HSTREAM stream, int count) {}
inline int AIL_stream_volume(HSTREAM stream) { return 0; }
inline void AIL_set_stream_volume(HSTREAM stream, int volume) {}
inline int AIL_stream_pan(HSTREAM stream) { return 0; }
inline void AIL_set_stream_pan(HSTREAM stream, int pan) {}
inline void AIL_close_stream(HSTREAM stream) {}
inline void AIL_pause_stream(HSTREAM stream, int onoff) {}
inline AIL_stream_callback AIL_register_stream_callback(HSTREAM stream, AIL_stream_callback callback) { return callback; }
inline AIL_3dsample_callback AIL_register_3D_EOS_callback(H3DSAMPLE sample, AIL_3dsample_callback EOS) { return EOS; }
inline AIL_sample_callback AIL_register_EOS_callback(HSAMPLE sample, AIL_sample_callback EOS) { return EOS; }
inline void AIL_start_stream(HSTREAM stream) {}
inline HSTREAM AIL_open_stream_by_sample(HDIGDRIVER driver, HSAMPLE sample, const char* file_name, int mem) { return nullptr; }
inline void AIL_set_sample_playback_rate(HSAMPLE sample, int playback_rate) {}
inline int AIL_sample_playback_rate(HSAMPLE sample) { return 0; }
inline void AIL_sample_ms_position(HSAMPLE sample, long* total_ms, long* current_ms) {}
inline void AIL_set_sample_ms_position(HSAMPLE sample, int pos) {}
inline int AIL_sample_loop_count(HSAMPLE sample) { return 0; }
inline void AIL_set_sample_loop_count(HSAMPLE sample, int count) {}
inline int AIL_sample_volume(HSAMPLE sample) { return 0; }
inline void AIL_set_sample_volume(HSAMPLE sample, int volume) {}
inline int AIL_sample_pan(HSAMPLE sample) { return 0; }
inline void AIL_set_sample_pan(HSAMPLE sample, int pan) {}
inline void AIL_end_sample(HSAMPLE sample) {}
inline void AIL_resume_sample(HSAMPLE sample) {}
inline void AIL_stop_sample(HSAMPLE sample) {}
inline void AIL_start_sample(HSAMPLE sample) {}
inline void AIL_init_sample(HSAMPLE sample) {}
inline int AIL_set_named_sample_file(HSAMPLE sample, const char* file_name, const void* file_image, int file_size, int block)  { return 0; }
inline void AIL_set_3D_sample_effects_level(H3DSAMPLE sample, float effect_level) {}
inline void AIL_set_3D_sample_distances(H3DSAMPLE sample, float max_dist, float min_dist) {}
inline void AIL_set_3D_velocity_vector(H3DSAMPLE sample, float x, float y, float z) {}
inline void AIL_set_3D_position(H3DPOBJECT obj, float X, float Y, float Z) {}
inline void AIL_set_3D_orientation(H3DPOBJECT obj, float X_face, float Y_face, float Z_face, float X_up, float Y_up, float Z_up) {}
inline int AIL_WAV_info(const void* data, AILSOUNDINFO* info) { return 0; }
inline void AIL_stop_timer(HTIMER timer) {}
inline void AIL_release_timer_handle(HTIMER timer) {}
inline void AIL_shutdown(void) {}
inline int AIL_enumerate_filters(HPROENUM* next, HPROVIDER* dest, char** name) { return 0; }
inline void AIL_set_file_callbacks(AIL_file_open_callback opencb, AIL_file_close_callback closecb, AIL_file_seek_callback seekcb, AIL_file_read_callback readcb) {}
inline void AIL_release_3D_sample_handle(H3DSAMPLE sample) {}
inline H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER lib) { return nullptr; }
inline void AIL_set_3D_user_data(H3DPOBJECT obj, unsigned int index, int value) {}
inline void AIL_unlock(void) {}
inline void AIL_lock(void) {}
inline void AIL_set_3D_speaker_type(HPROVIDER lib, int speaker_type) {}
inline void AIL_close_3D_listener(H3DPOBJECT listener) {}
inline int AIL_enumerate_3D_providers(HPROENUM* next, HPROVIDER* dest, char** name) { return 0; }
inline M3DRESULT AIL_open_3D_provider(HPROVIDER lib) { return 0; }
inline char* AIL_last_error(void) { return nullptr; }
inline H3DPOBJECT AIL_open_3D_listener(HPROVIDER lib) { return nullptr; }
inline uintptr_t AIL_3D_user_data(H3DSAMPLE sample, int index) { return 0; }
inline uintptr_t AIL_sample_user_data(HSAMPLE sample, int index) { return 0; }
inline HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER dig) { return nullptr; }
inline void AIL_set_sample_user_data(HSAMPLE sample, unsigned int index, int value) {}
inline int AIL_decompress_ADPCM(const AILSOUNDINFO* info, void** outdata, unsigned long* outsize) { return 0; }
inline void AIL_get_DirectSound_info(HSAMPLE sample, AILLPDIRECTSOUND* lplpDS, AILLPDIRECTSOUNDBUFFER* lplpDSB) {}
inline void AIL_mem_free_lock(void* ptr) {}
inline HSTREAM AIL_open_stream(HDIGDRIVER dig, const char* filename, int stream_mem) { return nullptr; }
inline int AIL_startup(void) { return 0; }
inline void AIL_quick_unload(HAUDIO audio) {}
inline HAUDIO AIL_quick_load_and_play(const char* filename, unsigned int loop_count, int wait_request) { return nullptr; }
inline void AIL_quick_set_volume(HAUDIO audio, float volume, float extravol) {}
inline int AIL_quick_startup(int use_digital, int use_MIDI, unsigned int output_rate, int output_bits, int output_channels) { return 0; }
inline void AIL_quick_handles(HDIGDRIVER* pdig, HMDIDRIVER* pmdi, HDLSDEVICE* pdls) {}
inline void AIL_sample_volume_pan(HSAMPLE sample, float* volume, float* pan) {}
inline void AIL_set_3D_sample_occlusion(H3DSAMPLE sample, float occlusion) {}
inline char* AIL_set_redist_directory(const char* dir) { return nullptr;  }
inline int AIL_set_sample_file(HSAMPLE sample, const void* file_image, int block) { return 0; }
inline void AIL_set_sample_volume_pan(HSAMPLE sample, float volume, float pan) {}
inline void AIL_set_stream_volume_pan(HSTREAM stream, float volume, float pan) {}
inline void AIL_stream_volume_pan(HSTREAM stream, float* volume, float* pan) {}
inline unsigned long AIL_get_timer_highest_delay(void) { return 0; }
inline int MSS_auto_cleanup(void) { return 0; }

#if !defined BUILD_STUBS
#define AIL_startup() (MSS_auto_cleanup(), AIL_startup())
#endif

#endif //_MILES_STUB_HO_INCLUDED_