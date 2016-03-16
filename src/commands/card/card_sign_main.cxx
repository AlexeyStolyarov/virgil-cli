/**
 * Copyright (C) 2015 Virgil Security Inc.
 *
 * Lead Maintainer: Virgil Security Inc. <support@virgilsecurity.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *     (3) Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>
#include <stdexcept>

#include <tclap/CmdLine.h>

#include <virgil/sdk/ServicesHub.h>
#include <virgil/sdk/models/SignModel.h>
#include <virgil/sdk/io/Marshaller.h>

#include <cli/version.h>
#include <cli/config.h>
#include <cli/pair.h>
#include <cli/util.h>

namespace vsdk = virgil::sdk;
namespace vcrypto = virgil::crypto;
namespace vcli = virgil::cli;

#ifdef SPLIT_CLI
#define MAIN main
#else
#define MAIN card_sign_main
#endif

int MAIN(int argc, char** argv) {
    try {
        std::string description = "Sign a Card\n";

        std::vector<std::string> examples;
        examples.push_back(
            "Alice is signing Bob's Card:\n"
            "virgil card-sign -s alice.vcard -b bob.vcard -k alice/private.vkey\n");

        std::string descriptionMessage = virgil::cli::getDescriptionMessage(description, examples);

        // Parse arguments.
        TCLAP::CmdLine cmd(descriptionMessage, ' ', virgil::cli_version());

        TCLAP::ValueArg<std::string> outArg("o", "out", "Card Sign. If omitted, stdout is used.", false, "", "file");

        TCLAP::ValueArg<std::string> signerArg("s", "signer", "Signer's Card", true, "", "file");

        TCLAP::ValueArg<std::string> toBeSignedArg("b", "to-be-signed", "Card to be signed.", true, "", "file");

        TCLAP::ValueArg<std::string> privateKeyArg("k", "key", "Signer's Private key", true, "", "file");

        cmd.add(privateKeyArg);
        cmd.add(toBeSignedArg);
        cmd.add(signerArg);
        cmd.add(outArg);
        cmd.parse(argc, argv);

        vsdk::models::CardModel signerCard = vcli::readCard(signerArg.getValue());
        vsdk::models::CardModel toBeSignedCard = vcli::readCard(toBeSignedArg.getValue());

        std::string pathPrivateKey = privateKeyArg.getValue();
        vcrypto::VirgilByteArray privateKey = vcli::readFileBytes(pathPrivateKey);
        vcrypto::VirgilByteArray privateKeyPass = vcli::setPrivateKeyPass(privateKey);
        vsdk::Credentials credentials(privateKey, privateKeyPass);

        vsdk::ServicesHub servicesHub(VIRGIL_ACCESS_TOKEN);
        vsdk::models::SignModel cardSign =
            servicesHub.card().sign(toBeSignedCard.getId(), toBeSignedCard.getHash(), signerCard.getId(), credentials);

        std::string cardSignStr = vsdk::io::Marshaller<vsdk::models::SignModel>::toJson<4>(cardSign);
        vcli::writeBytes(outArg.getValue(), cardSignStr);

        std::cout << "Card with card-id: " << signerCard.getId()
                  << " has been used to sign the Card with card-id: " << toBeSignedCard.getId() << std::endl;

    } catch (TCLAP::ArgException& exception) {
        std::cerr << "card-sign. Error: " << exception.error() << " for arg " << exception.argId() << std::endl;
        return EXIT_FAILURE;
    } catch (std::exception& exception) {
        std::cerr << "card-sign. Error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
