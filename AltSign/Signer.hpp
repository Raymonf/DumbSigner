//
//  Signer.hpp
//  AltSign-Windows
//
//  Created by Riley Testut on 8/12/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#ifndef Signer_hpp
#define Signer_hpp

#include <memory>
#include <string>
#include <vector>

#include "Certificate.hpp"
#include "ProvisioningProfile.hpp"

class Signer {
public:
    Signer(std::shared_ptr<Certificate> certificate);
    ~Signer();

    // std::shared_ptr<Team> team() const;
    std::shared_ptr<Certificate> certificate() const;

    void SignApp(std::string appPath, std::shared_ptr<ProvisioningProfile> profile);

private:
    std::shared_ptr<Certificate> _certificate;
};

#endif /* Signer_hpp */
