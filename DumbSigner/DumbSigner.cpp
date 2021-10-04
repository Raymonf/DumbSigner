#include "shared.h"
#include "ProvisioningProfile.hpp"
#include "Signer.hpp"
#include <cxxopts.hpp>

int main(int argc, char* argv[])
{
    cxxopts::Options options("DumbSigner", "An IPA signer heavily based on AltSign");

    options.add_options()
        ("a,app", "Path to the IPA file to resign", cxxopts::value<std::string>())
        ("c,cert", "Path to the certificate in .p12 (PKCS #12) format", cxxopts::value<std::string>())
        ("p,password", "Password for the certificate", cxxopts::value<std::string>())
        ("m,profile", "Mobile provisioning profile file in .mobileprovision format", cxxopts::value<std::string>())
        ("h,help", "Print help information");

    auto result = options.parse(argc, argv);

    if (result.count("help") || !(result.count("cert") || result.count("password") || result.count("profile") || result.
        count("app"))) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    try {
        auto p12Data = readFile(result["cert"].as<std::string>().c_str());
        auto certificate = std::make_shared<Certificate>(p12Data, result["password"].as<std::string>().c_str());
        auto profileData = readFile(result["profile"].as<std::string>().c_str());

        auto appPath = result["app"].as<std::string>();
        std::cout << "Attempting to sign " << appPath << std::endl;

        auto signer = Signer(certificate);
        auto profile = std::make_shared<ProvisioningProfile>(profileData);
        signer.SignApp(appPath, profile);
    } catch (std::exception& ex) {
        std::cout << "oh no! it blew up! exception message was: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
