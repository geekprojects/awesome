
#include <awesome/cursor.h>
#include <geek/core-data.h>
#include <geek/gfx-surface.h>

#include <vector>
#include <cstring>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

#define X11_CURSOR_IMAGE 0xfffd0002

struct ImagePos
{
    int pos = -1;
    int size = -1;
};

Cursor::Cursor() = default;

Cursor::~Cursor() = default;

Cursor* Cursor::loadCursor()
{
    Data* cursorData = new Data();
    cursorData->load("../data/cursors/arrow");

    string magic = cursorData->readString(4);

    printf("loadCursor: magic=%s\n", magic.c_str());
    if (magic != "Xcur")
    {
        return nullptr;
    }

    int headerSize = cursorData->read32();
    int version = cursorData->read32();
    int ntoc = cursorData->read32();
    printf("loadCursor: headerSize=%d\n", headerSize);
    printf("loadCursor: version=%d\n", version);
    printf("loadCursor: ntoc=%d\n", ntoc);

    ImagePos imagePos;
    int i;
    int maxSize = -1;
    for (i = 0; i < ntoc; i++)
    {
        int type = cursorData->read32();
        int subtype = cursorData->read32();
        int position = cursorData->read32();
        printf("loadCursor: %d: type=0x%x, subtype=%d, position=%d\n", i, type, subtype, position);
        if (type == X11_CURSOR_IMAGE)
        {
            if (subtype > maxSize)
            {
                imagePos.pos = position;
                imagePos.size = subtype;
                maxSize = subtype;
            }
        }
    }

    Cursor* cursor = nullptr;
    if (maxSize > 0)
    {
        cursorData->reset();
        cursorData->setPos(imagePos.pos);

        /*int header =*/ cursorData->read32();
        int type = cursorData->read32();
        int subtype = cursorData->read32();
        /*int version =*/ cursorData->read32();
        int width = cursorData->read32();
        int height = cursorData->read32();
        int xpos = cursorData->read32();
        int ypos = cursorData->read32();
        int delay = cursorData->read32();
        printf("loadCursor: image %d: type=0x%x, subtype=%d, version=%d, width=%d, height=%d, xpos=%d, ypos=%d, delay=%d\n", i, type, subtype, version, width, height, xpos, ypos, delay);

        cursor = new Cursor();
        Surface* surface = new Surface(width, height, 4);
        cursor->m_surface = surface;
        memcpy(surface->getData(), cursorData->posPointer(), surface->getDataLength());

        uint8_t* pos = surface->getData();
        for (; pos < surface->getData() + surface->getDataLength(); pos += 4)
        {
            uint8_t a = pos[3];
            /* if (a == 0)
            {
                pos[0] = 255;
                pos[1] = 0;
            } */
            //pos[3] = 255 - pos[3];
            //pos[0] = a;
            //pos[1] = a;
            //pos[2] = a;
            if (a == 0)
            {
                pos[3] = 1;
            }
            //pos[3] = 255;
        }

        cursor->m_surface->saveJPEG("cursor.jpg");
        cursor->m_hotSpot.x = xpos;
        cursor->m_hotSpot.y = ypos;
    }

    delete cursorData;

    return cursor;
}

