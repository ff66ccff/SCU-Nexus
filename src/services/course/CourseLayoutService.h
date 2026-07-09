#ifndef COURSELAYOUTSERVICE_H
#define COURSELAYOUTSERVICE_H

#include <QList>
#include <QHash>
#include "../../models/Course.h"

namespace SCUNexus {

// Computes layout metadata for course display in the weekly grid:
// - which courses are visible for a given week
// - track assignment for handling conflicts (side-by-side display)
// - merging same-slot courses
class CourseLayoutService {
public:
    // Represents a course with layout metadata for the grid
    struct LayoutCourse {
        Course course;
        int track = 0;        // 0-based track index for side-by-side display
        int totalTracks = 1;  // total tracks in the conflict group
        bool active = true;   // whether active in the current display week
        bool conflict = false; // whether it conflicts with another course
    };

    // Select visible courses for a given day, considering the display week
    // and whether to show non-current-week courses
    static QList<LayoutCourse> selectVisibleCoursesForDay(
        const QList<Course>& allCourses,
        int dayOfWeek,
        int displayWeek,
        bool showNonCurrentWeekCourses);

    // Assign tracks to courses on the same day to handle overlaps
    static void assignCourseTracks(QList<LayoutCourse>& dayCourses);

    // Merge courses that share the same slot (same name, day, sections, location)
    static QList<Course> mergeSameSlotCourses(const QList<Course>& courses);

    // Sort courses by layout order: startSection asc, duration desc, startWeek asc
    static void sortForLayout(QList<LayoutCourse>& courses);

    // Get all courses visible for the entire week
    static QList<LayoutCourse> selectVisibleCoursesForWeek(
        const QList<Course>& allCourses,
        int displayWeek,
        bool showNonCurrentWeekCourses);
};

} // namespace SCUNexus

#endif // COURSELAYOUTSERVICE_H
