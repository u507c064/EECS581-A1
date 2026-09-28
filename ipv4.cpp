#include <iostream>
#include <string>
#include <cstdint>

using namespace std;

/*
 * Determines whether a character can belong to an IPv4 candidate token.
 *
 * Candidate tokens may contain only:
 *   - digits
 *   - periods
 *   - colons
 */
bool isCandidateCharacter(char c)
{
    return (c >= '0' && c <= '9') || c == '.' || c == ':';
}

/*
 * Validates one complete candidate token.
 *
 * The token is located in str[start] through str[end - 1].
 *
 * On success:
 *   address = 32-bit decimal IPv4 value
 *   port    = port number, or -1 if no port exists
 *
 * On failure:
 *   returns false
 */
bool parseCandidate(const string& str,
                    size_t start,
                    size_t end,
                    uint32_t& address,
                    int& port)
{
    size_t pos = start;

    unsigned int octets[4];

    // ------------------------------------------------------------
    // Parse exactly four IPv4 octets.
    // ------------------------------------------------------------
    for (int i = 0; i < 4; i++)
    {
        // There must be at least one character for this octet.
        if (pos >= end)
        {
            return false;
        }

        // The first character of an octet must be a digit.
        if (str[pos] < '0' || str[pos] > '9')
        {
            return false;
        }

        // Leading zeroes are not allowed.
        if (str[pos] == '0' &&
            pos + 1 < end &&
            str[pos + 1] >= '0' &&
            str[pos + 1] <= '9')
        {
            return false;
        }

        unsigned int value = 0;
        int digitCount = 0;

        // Manually accumulate the octet value.
        while (pos < end &&
               str[pos] >= '0' &&
               str[pos] <= '9')
        {
            // An octet may contain at most three digits.
            if (digitCount == 3)
            {
                return false;
            }

            value = value * 10 +
                    static_cast<unsigned int>(str[pos] - '0');

            digitCount++;
            pos++;
        }

        // An octet must contain at least one digit.
        if (digitCount == 0)
        {
            return false;
        }

        // Octet values must be between 0 and 255.
        if (value > 255)
        {
            return false;
        }

        octets[i] = value;

        // The first three octets must be followed by a period.
        if (i < 3)
        {
            if (pos >= end || str[pos] != '.')
            {
                return false;
            }

            pos++;
        }
    }

    // ------------------------------------------------------------
    // Parse the optional port.
    // ------------------------------------------------------------

    // No characters remain: there is no port.
    if (pos == end)
    {
        port = -1;
    }
    else
    {
        // If anything remains after the IPv4 address,
        // it must begin with a colon.
        if (str[pos] != ':')
        {
            return false;
        }

        pos++;

        // A colon must be followed by a complete port.
        if (pos >= end)
        {
            return false;
        }

        // The first port character must be a digit.
        if (str[pos] < '0' || str[pos] > '9')
        {
            return false;
        }

        // Leading zeroes are not allowed for ports.
        if (str[pos] == '0' &&
            pos + 1 < end &&
            str[pos + 1] >= '0' &&
            str[pos + 1] <= '9')
        {
            return false;
        }

        unsigned int portValue = 0;
        int digitCount = 0;

        // Manually accumulate the port value.
        while (pos < end &&
               str[pos] >= '0' &&
               str[pos] <= '9')
        {
            // A port may contain at most five digits.
            if (digitCount == 5)
            {
                return false;
            }

            portValue = portValue * 10 +
                        static_cast<unsigned int>(str[pos] - '0');

            digitCount++;
            pos++;
        }

        // The entire candidate token must have been consumed.
        if (pos != end)
        {
            return false;
        }

        // Port values must be between 0 and 65535.
        if (portValue > 65535)
        {
            return false;
        }

        port = static_cast<int>(portValue);
    }

    // ------------------------------------------------------------
    // Construct the 32-bit IPv4 value.
    //
    // Casting to uint32_t before shifting prevents problems
    // associated with signed integer arithmetic.
    // ------------------------------------------------------------
    address =
        (static_cast<uint32_t>(octets[0]) << 24) |
        (static_cast<uint32_t>(octets[1]) << 16) |
        (static_cast<uint32_t>(octets[2]) << 8)  |
        static_cast<uint32_t>(octets[3]);

    return true;
}

/*
 * Searches the input string for exactly one valid IPv4 address.
 */
bool extractIPv4(const string& str,
                 unsigned long& outAddress,
                 int& outPort)
{
    // Required failure values.
    outAddress = 0;
    outPort = -1;

    size_t pos = 0;

    bool foundAddress = false;
    uint32_t foundAddressValue = 0;
    int foundPort = -1;

    while (pos < str.length())
    {
        // Skip characters that cannot belong to a candidate.
        if (!isCandidateCharacter(str[pos]))
        {
            pos++;
            continue;
        }

        // --------------------------------------------------------
        // Find the complete candidate token.
        //
        // The token continues until a character other than
        // digit, period, or colon is encountered.
        // --------------------------------------------------------
        size_t start = pos;

        while (pos < str.length() &&
               isCandidateCharacter(str[pos]))
        {
            pos++;
        }

        size_t end = pos;

        // --------------------------------------------------------
        // A token without a period or colon cannot be an IPv4
        // address, so there is no reason to validate it.
        // --------------------------------------------------------
        bool containsSeparator = false;

        for (size_t i = start; i < end; i++)
        {
            if (str[i] == '.' || str[i] == ':')
            {
                containsSeparator = true;
                break;
            }
        }

        if (!containsSeparator)
        {
            continue;
        }

        // --------------------------------------------------------
        // Validate the ENTIRE candidate token.
        // --------------------------------------------------------
        uint32_t candidateAddress = 0;
        int candidatePort = -1;

        if (parseCandidate(str,
                           start,
                           end,
                           candidateAddress,
                           candidatePort))
        {
            // We already found one valid IPv4 address.
            // Therefore, finding another means the input contains
            // more than exactly one valid address.
            if (foundAddress)
            {
                outAddress = 0;
                outPort = -1;
                return false;
            }

            foundAddress = true;
            foundAddressValue = candidateAddress;
            foundPort = candidatePort;
        }
    }

    // Exactly one valid address was required.
    if (!foundAddress)
    {
        outAddress = 0;
        outPort = -1;
        return false;
    }

    outAddress = static_cast<unsigned long>(foundAddressValue);
    outPort = foundPort;

    return true;
}

/*
 * Main program.
 */
int main()
{
    string input;

    while (true)
    {
        cout << "Enter a string (or 'END' to quit): ";

        getline(cin, input);

        // Terminate only on exactly "END".
        if (input == "END")
        {
            cout << "Program terminated." << endl;
            break;
        }

        unsigned long address = 0;
        int port = -1;

        if (extractIPv4(input, address, port))
        {
            /*
             * Reconstruct the four octets for display.
             *
             * The value returned by extractIPv4 represents the
             * address in the form:
             *
             * A.B.C.D
             */
            uint32_t address32 =
                static_cast<uint32_t>(address);

            unsigned int A =
                (address32 >> 24) & 0xFF;

            unsigned int B =
                (address32 >> 16) & 0xFF;

            unsigned int C =
                (address32 >> 8) & 0xFF;

            unsigned int D =
                address32 & 0xFF;

            cout << "Extracted IPv4 address: "
                 << A << "."
                 << B << "."
                 << C << "."
                 << D
                 << " (decimal value: "
                 << address
                 << ", port: ";

            if (port == -1)
            {
                cout << "none";
            }
            else
            {
                cout << port;
            }

            cout << ")" << endl;
        }
        else
        {
            cout << "Invalid input: no valid IPv4 address found"
                 << endl;
        }
    }

    return 0;
}