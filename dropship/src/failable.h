#pragma once

#include <string>
#include <format>

// disable override
// https://stackoverflow.com/a/16896559

//extern Debug debug;


class failable
{
    public:
        failable() {}
        virtual ~failable() {}

        virtual bool isFailed()
        final {
            return this->failed;
        }

        virtual std::string isFailedReason()
        final {
            return this->fail_reason;
        }

        virtual void fail(std::string why)
        final {
            this->failed = true;
            this->fail_reason = why;

            //debug->print(std::format("failed: {0}", why).c_str());
        }

    private:
        bool failed = false;
        std::string fail_reason = "";
};
