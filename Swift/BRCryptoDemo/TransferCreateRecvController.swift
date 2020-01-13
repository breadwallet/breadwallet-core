//
//  TransferCreateRecvController.swift
//  BRCryptoDemo
//
//  Created by Ed Gamble on 8/6/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import UIKit

class TransferCreateRecvController: TransferCreateController {

    let qrCodeSize = CGSize(width: 186.0, height: 186.0)

    override func viewDidLoad() {
        super.viewDidLoad()
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        updateView()

    }

    func updateView () {
        let target = wallet.target

        walletAddrButton.setTitle (target.description, for: .normal)

        // Generate a QR Code
        if let scheme = wallet.manager.network.scheme {
            let uri = "\(scheme):\(target.description)"
            qrCodeImage.isHidden = false
            qrCodeImage.image = UIImage
                .qrCode(data: uri.data(using: .utf8)!)!
                .resize(qrCodeSize)
        }
        else {
            qrCodeImage.isHidden = true
        }
    }

    @IBAction func done(_ sender: UIBarButtonItem) {
        self.dismiss(animated: true) {}
    }

    @IBAction func toPasteBoard(_ sender: UIButton) {
        UIPasteboard.general.string = sender.titleLabel?.text
    }

    @IBOutlet var walletAddrButton: UIButton!
    @IBOutlet var qrCodeImage: UIImageView!
}

// https://github.com/breadwallet/breadwallet-ios
extension UIImage {
    static func qrCode(data: Data,
                       color: CIColor = .black,
                       backgroundColor: CIColor = .white) -> UIImage? {
        guard let qrFilter = CIFilter(name: "CIQRCodeGenerator"),
            let colorFilter = CIFilter(name: "CIFalseColor") else { return nil }

        qrFilter.setDefaults()
        qrFilter.setValue(data, forKey: "inputMessage")
        qrFilter.setValue("L", forKey: "inputCorrectionLevel")

        colorFilter.setDefaults()
        colorFilter.setValue(qrFilter.outputImage, forKey: "inputImage")
        colorFilter.setValue(color, forKey: "inputColor0")
        colorFilter.setValue(backgroundColor, forKey: "inputColor1")

        guard let outputImage = colorFilter.outputImage else { return nil }
        guard let cgImage = CIContext().createCGImage(outputImage, from: outputImage.extent) else { return nil }
        return UIImage(cgImage: cgImage)
    }

    func resize(_ size: CGSize, inset: CGFloat = 6.0) -> UIImage? {
        UIGraphicsBeginImageContext(size)
        defer { UIGraphicsEndImageContext() }
        guard let context = UIGraphicsGetCurrentContext() else { assert(false, "Could not create image context"); return nil }
        guard let cgImage = self.cgImage else { assert(false, "No cgImage property"); return nil }

        context.interpolationQuality = .none
        context.rotate(by: .pi) // flip
        context.scaleBy(x: -1.0, y: 1.0) // mirror
        context.draw(cgImage, in: context.boundingBoxOfClipPath.insetBy(dx: inset, dy: inset))
        return UIGraphicsGetImageFromCurrentImageContext()
    }
}
