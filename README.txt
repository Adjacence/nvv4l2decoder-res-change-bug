Test code for nvv4l2decoder dynamic resolution change bug

This program will launch a pipeline with a videotestsrc at resolution 1280/720.
Once the pipeline has started playing, the program will attempt to change the
resolution to 720/1280, optionally stopping the pipeline in between the CAPS
change. The output of the videotestsrc is encoded and parsed as H264. The H264
data is decoded by nvv4l2decoder, then is re-encoded so it can be written out to
a matroska files. There is a splitmuxsink that handles writing out to files,
such that there is one matroska file per resolution.

Currently, the decoder produces a completely garbled output after the CAPS
change. I have confirmed that this program produces the expected result when
using a non-NVIDIA decoder.

Build Instructions:
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=<Release|Debug>
make

Run Instructions:
./nvv4l2decoder-res-change-bug [GSTREAMER_OPTIONS]
./nvv4l2decoder-res-change-bug [GSTREAMER_OPTIONS] stop-on-change
The latter option will stop the pipeline before performing the resolution change
