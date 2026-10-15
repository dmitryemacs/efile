#pragma once

class QApplication;

// Applies the dark ("black") theme to the whole application.
class Theme {
public:
    static void applyDark(QApplication& app);
};
