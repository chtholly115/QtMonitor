#ifndef TRACKER_H
#define TRACKER_H

#include "detection.h"
#include <QList>
#include <set>

class Tracker
{
public:
    explicit Tracker(float iouThreshold = 0.3f, int maxLost = 5);

    QList<Detection> update(const QList<Detection> &detections);
    void reset();

private:
    struct Track
    {
        int id = -1;
        float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
        int classId = -1;
        float score = 0.0f;
        int lost = 0;
    };

    static float iou(const Detection &det, const Track &track);

    QList<Track> tracks_;
    int nextId_ = 1;
    std::set<int> availableIds_;   // 空闲 ID 池（自动排序）
    float iouThreshold_;
    int maxLost_;
};

#endif // TRACKER_H