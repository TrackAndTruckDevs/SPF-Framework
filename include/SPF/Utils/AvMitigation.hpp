#pragma once

namespace SPF::Utils {
    /**
    * @brief Function to reduce the probability of false positives from antiviruses.
    * Adds calls to standard Windows APIs and references a block of text to change the file's entropy.
    */
    void InitializeAvMitigation();
}
