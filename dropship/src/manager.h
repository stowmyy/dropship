#pragma once


// disable override
// https://stackoverflow.com/a/16896559


class manager
{
    public:
        manager () : window_open(true) {}
        virtual ~manager() {
            // delete window_open;
        }
        bool window_open;

    private:
};
