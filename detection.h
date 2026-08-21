#ifndef DETECTION_H
#define DETECTION_H

#include <QMetaType>

struct Detection
{
    float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
    int classId = -1;
    float score = 0.0f;
    int trackId = -1;
};

Q_DECLARE_METATYPE(Detection)

#endif // DETECTION_H
