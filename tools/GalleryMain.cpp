// Entry point of the control gallery. The work happens in Gallery.cpp, which is compiled into the
// plug-in's shared code (ga_add_gallery), so it sees exactly the same JUCE configuration.
int gaRenderGallery (int argc, char** argv);

int main (int argc, char** argv)
{
    return gaRenderGallery (argc, argv);
}
