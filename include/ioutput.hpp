/*
    File: ioutput.hpp
*/

/*
    Interface: IOutput
        Base interface for the output channel needed when logging infos or warnings, This is used in order
        to allow the user to use his own custom logging facility, he just needs this as a layer.

    Note:
        When running in Release mode or with the Ship flag enabled nothing will be logged
*/
class IOutput
{
public :
	virtual ~IOutput() = default;

    /*
        Function: log_info
            Logs the specified messaged tagged as info
    */
    virtual void log_info(const char* message) = 0;

    /*
        Function: log_warning
            Logs the specified messaged tagged as warning
    */
    virtual void log_warning(const char* message) = 0;

    /*
        Function: log_error
            Logs the specified messaged tagged as error
    */
    virtual void log_error(const char* message) = 0;
};