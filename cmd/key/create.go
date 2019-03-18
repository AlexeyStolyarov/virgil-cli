/*
 * Copyright (C) 2015-2019 Virgil Security Inc.
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
 *
 * Lead Maintainer: Virgil Security Inc. <support@virgilsecurity.com>
 */

package key

import (
	"encoding/base64"
	"fmt"
	"gopkg.in/virgilsecurity/virgil-crypto-go.v5"
	"net/http"

	"github.com/VirgilSecurity/virgil-cli/utils"

	"github.com/VirgilSecurity/virgil-cli/models"

	"github.com/VirgilSecurity/virgil-cli/client"
	"github.com/pkg/errors"
	"gopkg.in/urfave/cli.v2"
)

var (
	crypto = virgil_crypto_go.NewVirgilCrypto()
)

func Create(vcli *client.VirgilHttpClient) *cli.Command {
	return &cli.Command{
		Name:      "create",
		Aliases:   []string{"c"},
		ArgsUsage: "api-key name",
		Usage:     "Create a new api-key",
		Flags:     []cli.Flag{&cli.StringFlag{Name: "name"}, &cli.StringFlag{Name: "app_id"}},
		Action: func(context *cli.Context) (err error) {

			name := context.String("name")

			if name == "" {
				if context.NArg() < 1 {
					return errors.New("Invalid number of arguments. Please, specify api-key name")
				} else {
					name = context.Args().First()
				}
			}

			appID := context.String("app_id")
			if appID == "" {
				appID, _ := utils.LoadAppID()
				if appID == "" {
					return errors.New("Please, specify app_id (flag --app_id)")
				}
			} else {
				utils.SaveAppID(appID)
			}

			var apiKeyID string
			apiKeyID, err = CreateFunc(name, vcli)

			if err != nil {
				return err
			}

			fmt.Println("API_KEY_ID:", apiKeyID)
			return nil
		},
	}
}

func CreateFunc(name string, vcli *client.VirgilHttpClient) (apiKeyID string, err error) {

	keyPair, err := crypto.GenerateKeypair()

	if err != nil {
		return "", err
	}

	prKey, err := crypto.ExportPrivateKey(keyPair.PrivateKey(), "")
	if err != nil {
		return "", err
	}
	pubKey, err := crypto.ExportPublicKey(keyPair.PublicKey())
	if err != nil {
		return "", err
	}
	sign, err := crypto.Sign(pubKey, keyPair.PrivateKey())
	if err != nil {
		return "", err
	}
	req := &models.CreateAccessKeyRequest{Name: name, PublicKey: pubKey, Signature: sign}
	resp := &models.AccessKey{}

	token, err := utils.LoadAccessTokenOrLogin(vcli)

	if err != nil {
		return "", err
	}

	for err == nil {
		_, _, vErr := vcli.Send(http.MethodPost, token, "access_keys", req, resp)
		if vErr == nil {
			break
		}

		token, err = utils.CheckRetry(vErr, vcli)
	}

	if err != nil {
		return
	}

	if resp != nil {

		fmt.Println("API_KEY:", base64.StdEncoding.EncodeToString(prKey))

		return resp.ID, nil
	}

	return "", errors.New("empty response")
}
