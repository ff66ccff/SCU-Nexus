#include "CourseLayoutService.h"
#include <algorithm>
#include <set>

namespace SCUNexus {

QList<CourseLayoutService::LayoutCourse>
CourseLayoutService::selectVisibleCoursesForDay(
    const QList<Course>& allCourses,
    int dayOfWeek,
    int displayWeek,
    bool showNonCurrentWeekCourses)
{
    QList<LayoutCourse> result;

    // Collect courses for this day
    QList<Course> dayCourseList;
    for (const auto& course : allCourses) {
        if (course.dayOfWeek != dayOfWeek) continue;
        dayCourseList.append(course);
    }

    if (!showNonCurrentWeekCourses) {
        // Only show courses active in the current display week
        for (const auto& course : dayCourseList) {
            LayoutCourse lc;
            lc.course = course;
            lc.active = course.isActiveInWeek(displayWeek);
            if (lc.active) {
                result.append(lc);
            }
        }
    } else {
        // Show courses in week range, plus future courses that don't conflict
        // First, collect courses already in range
        QList<Course> inRangeCourses;
        QList<Course> futureCourses;

        for (const auto& course : dayCourseList) {
            if (course.endWeek < displayWeek && course.startWeek < displayWeek) {
                // Course has already ended, skip unless it spans across
                if (course.startWeek <= displayWeek && course.endWeek >= displayWeek) {
                    inRangeCourses.append(course);
                }
                continue;
            }
            if (course.startWeek > displayWeek) {
                // Future course
                if (course.isInWeekRange(displayWeek) || course.startWeek > displayWeek) {
                    futureCourses.append(course);
                }
                continue;
            }
            inRangeCourses.append(course);
        }

        // Add in-range courses
        for (const auto& course : inRangeCourses) {
            LayoutCourse lc;
            lc.course = course;
            lc.active = course.isActiveInWeek(displayWeek);
            result.append(lc);
        }

        // Add future courses that don't conflict with visible in-range courses
        for (const auto& future : futureCourses) {
            bool hasConflict = false;
            for (const auto& lc : result) {
                const bool sectionsOverlap = future.startSection <= lc.course.endSection
                    && lc.course.startSection <= future.endSection;
                if (lc.active && sectionsOverlap) {
                    hasConflict = true;
                    break;
                }
            }
            if (!hasConflict) {
                LayoutCourse lc;
                lc.course = future;
                lc.active = false; // Not active yet, but shown for planning
                result.append(lc);
            }
        }
    }

    sortForLayout(result);
    assignCourseTracks(result);

    return result;
}

QList<CourseLayoutService::LayoutCourse>
CourseLayoutService::selectVisibleCoursesForWeek(
    const QList<Course>& allCourses,
    int displayWeek,
    bool showNonCurrentWeekCourses)
{
    QList<LayoutCourse> result;

    // Resolve each day independently. This keeps future-course filtering
    // deterministic regardless of the input order and assigns tracks only
    // among courses that can share a column.
    for (int day = 1; day <= 7; ++day) {
        result.append(selectVisibleCoursesForDay(allCourses, day, displayWeek,
                                                 showNonCurrentWeekCourses));
    }

    sortForLayout(result);
    return result;
}

void CourseLayoutService::assignCourseTracks(QList<LayoutCourse>& dayCourses) {
    if (dayCourses.isEmpty()) return;

    // Sort by start section ascending
    std::sort(dayCourses.begin(), dayCourses.end(),
              [](const LayoutCourse& a, const LayoutCourse& b) {
                  if (a.course.startSection != b.course.startSection)
                      return a.course.startSection < b.course.startSection;
                  if (a.course.endSection != b.course.endSection)
                      return a.course.endSection > b.course.endSection;
                  return a.course.id < b.course.id;
              });

    // Assign tracks: for each course, find the first available track
    // among courses whose section ranges overlap with it
    for (int i = 0; i < dayCourses.size(); ++i) {
        // Find which tracks are occupied by overlapping courses
        std::set<int> occupiedTracks;

        for (int j = 0; j < i; ++j) {
            const auto& other = dayCourses[j];
            // Check if sections overlap
            if (dayCourses[i].course.startSection <= other.course.endSection &&
                other.course.startSection <= dayCourses[i].course.endSection) {
                occupiedTracks.insert(other.track);
            }
        }

        // Find first free track
        int track = 0;
        while (occupiedTracks.count(track) > 0) {
            track++;
        }

        dayCourses[i].track = track;
    }

    // Give every course in one connected overlap group the same denominator.
    // For example [1-2], [2-3], [3-4] needs tracks 0,1,0 and a width of 1/2
    // for all three cards; direct pair counts would incorrectly produce 2,3,2.
    int groupStart = 0;
    while (groupStart < dayCourses.size()) {
        int groupEnd = groupStart;
        int coveredUntil = dayCourses[groupStart].course.endSection;
        int totalTracks = dayCourses[groupStart].track + 1;
        while (groupEnd + 1 < dayCourses.size()
               && dayCourses[groupEnd + 1].course.startSection <= coveredUntil) {
            ++groupEnd;
            coveredUntil = std::max(coveredUntil,
                                    dayCourses[groupEnd].course.endSection);
            totalTracks = std::max(totalTracks, dayCourses[groupEnd].track + 1);
        }

        const bool conflict = groupEnd > groupStart;
        for (int i = groupStart; i <= groupEnd; ++i) {
            dayCourses[i].totalTracks = totalTracks;
            dayCourses[i].conflict = conflict;
        }
        groupStart = groupEnd + 1;
    }
}

void CourseLayoutService::sortForLayout(QList<LayoutCourse>& courses) {
    std::sort(courses.begin(), courses.end(),
              [](const LayoutCourse& a, const LayoutCourse& b) {
                  // 1. Start section ascending
                  if (a.course.startSection != b.course.startSection)
                      return a.course.startSection < b.course.startSection;
                  // 2. Duration descending (longer courses first)
                  int aDur = a.course.endSection - a.course.startSection;
                  int bDur = b.course.endSection - b.course.startSection;
                  if (aDur != bDur)
                      return aDur > bDur;
                  // 3. Start week ascending
                  return a.course.startWeek < b.course.startWeek;
              });
}

QList<Course> CourseLayoutService::mergeSameSlotCourses(const QList<Course>& courses) {
    if (courses.size() <= 1) return courses;

    QList<Course> merged;
    QList<bool> mergedFlag(courses.size(), false);

    for (int i = 0; i < courses.size(); ++i) {
        if (mergedFlag[i]) continue;

        Course base = courses[i];
        QStringList teachers;
        QStringList locations;
        if (!base.teacher.isEmpty()) teachers.append(base.teacher);
        if (!base.location.isEmpty()) locations.append(base.location);

        int minStartWeek = base.startWeek;
        int maxEndWeek = base.endWeek;
        WeekType combinedWeekType = base.weekType;

        for (int j = i + 1; j < courses.size(); ++j) {
            if (mergedFlag[j]) continue;

            const Course& other = courses[j];

            // Merge conditions: same name, same day, same section range, same location
            if (other.name != base.name) continue;
            if (other.dayOfWeek != base.dayOfWeek) continue;
            if (other.startSection != base.startSection) continue;
            if (other.endSection != base.endSection) continue;
            if (other.location != base.location) continue;

            mergedFlag[j] = true;

            // Merge teacher (deduplicate)
            if (!other.teacher.isEmpty() && !teachers.contains(other.teacher)) {
                teachers.append(other.teacher);
            }

            // Merge location (deduplicate)
            if (!other.location.isEmpty() && !locations.contains(other.location)) {
                locations.append(other.location);
            }

            // Expand week range
            minStartWeek = std::min(minStartWeek, other.startWeek);
            maxEndWeek = std::max(maxEndWeek, other.endWeek);

            // If week types differ, set to Every
            if (combinedWeekType != other.weekType) {
                combinedWeekType = WeekType::Every;
            }
        }

        base.teacher = teachers.join(QStringLiteral("、"));
        base.location = locations.join(QStringLiteral("、"));
        base.startWeek = minStartWeek;
        base.endWeek = maxEndWeek;
        base.weekType = combinedWeekType;

        merged.append(base);
    }

    return merged;
}

} // namespace SCUNexus
