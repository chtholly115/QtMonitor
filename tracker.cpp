// tracker.cpp
#include "tracker.h"

#include <algorithm>
#include <vector>

Tracker::Tracker(float iouThreshold, int maxLost)
    : iouThreshold_(iouThreshold)
    , maxLost_(maxLost)
{
}

float Tracker::iou(const Detection &det, const Track &track)
{
    const float x1 = std::max(det.x1, track.x1);
    const float y1 = std::max(det.y1, track.y1);
    const float x2 = std::min(det.x2, track.x2);
    const float y2 = std::min(det.y2, track.y2);

    const float interW = std::max(0.0f, x2 - x1);
    const float interH = std::max(0.0f, y2 - y1);
    const float interArea = interW * interH;

    const float detArea = std::max(0.0f, det.x2 - det.x1) * std::max(0.0f, det.y2 - det.y1);
    const float trackArea = std::max(0.0f, track.x2 - track.x1) * std::max(0.0f, track.y2 - track.y1);
    const float unionArea = detArea + trackArea - interArea;

    if (unionArea <= 0.0f)
        return 0.0f;

    return interArea / unionArea;
}

QList<Detection> Tracker::update(const QList<Detection> &detections)
{
    QList<Detection> result = detections;
    const int detectionCount = result.size();
    const int trackCount = tracks_.size();

    std::vector<bool> detectionMatched(detectionCount, false);
    std::vector<bool> trackMatched(trackCount, false);

    QList<Track> newTracks;

    // 1. 对每个检测框，在同类别且未匹配的旧轨迹中找 IoU 最大的匹配
    for (int di = 0; di < detectionCount; ++di) {
        Detection &det = result[di];
        int bestTrackIdx = -1;
        float bestIou = iouThreshold_;

        for (int ti = 0; ti < trackCount; ++ti) {
            if (trackMatched[ti])
                continue;

            const Track &track = tracks_[ti];
            if (track.classId != det.classId)
                continue;

            const float currentIou = iou(det, track);
            if (currentIou > bestIou) {
                bestIou = currentIou;
                bestTrackIdx = ti;
            }
        }

        if (bestTrackIdx != -1) {
            detectionMatched[di] = true;
            trackMatched[bestTrackIdx] = true;
            det.trackId = tracks_[bestTrackIdx].id;

            Track updated = tracks_[bestTrackIdx];
            updated.x1 = det.x1;
            updated.y1 = det.y1;
            updated.x2 = det.x2;
            updated.y2 = det.y2;
            updated.classId = det.classId;
            updated.score = det.score;
            updated.lost = 0;

            newTracks.append(updated);
        }
    }

    // 2. 未匹配的检测框 -> 新轨迹
    for (int di = 0; di < detectionCount; ++di) {
        if (detectionMatched[di])
            continue;

        Detection &det = result[di];

        int newId;
        if (!availableIds_.empty()) {
            newId = *availableIds_.begin();
            availableIds_.erase(availableIds_.begin());
        } else {
            newId = nextId_++;
        }
        det.trackId = newId;

        Track newTrack;
        newTrack.id = newId;
        newTrack.x1 = det.x1;
        newTrack.y1 = det.y1;
        newTrack.x2 = det.x2;
        newTrack.y2 = det.y2;
        newTrack.classId = det.classId;
        newTrack.score = det.score;
        newTrack.lost = 0;

        newTracks.append(newTrack);
    }

    // 3. 未匹配的旧轨迹：丢失计数增加，超限丢弃
    for (int ti = 0; ti < trackCount; ++ti) {
        if (trackMatched[ti])
            continue;

        Track oldTrack = tracks_[ti];
        oldTrack.lost++;
        if (oldTrack.lost <= maxLost_) {
            newTracks.append(oldTrack);
        } else {
            availableIds_.insert(oldTrack.id);
        }
    }

    tracks_ = newTracks;
    return result;
}

void Tracker::reset()
{
    tracks_.clear();
    availableIds_.clear();
    nextId_ = 1;
}