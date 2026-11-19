#ifndef ZCORE_H
#define ZCORE_H

#pragma once
#define NOMINMAX
#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

/**
 * @brief Highlights vein lines in the palm while ignoring flat skin and shadows.
 *
 * Under infrared light, veins look like dark rounded grooves or valleys under the skin.
 * This filter looks for those groove shapes across multiple vein thicknesses.
 * and turns them into a bright, clear vein map.
 *
 * @param ROI         The cropped palm image.
 * @param SigmaStart  Thinnest vein size to look for (default: 1.0).
 * @param SigmaEnd    Thickest vein size to look for (default: 3.0).
 * @param SigmaStep   Size step between passes (default: 1.0).
 * @return A black-and-white image where veins glow bright white and flat skin stays dark.
 */
inline cv::Mat FrangiFilter(
    const cv::Mat& ROI, float SigmaStart = 1.0f, float SigmaEnd = 3.0f, float SigmaStep = 1.0f
)
{
    cv::Mat Src;
    ROI.convertTo(Src, CV_32FC1, 1.0 / 255.0);

    const float Beta = 0.8f;  // Controls how strictly we reject round blobs instead of tube lines
    const float C    = 0.08f; // Controls sensitivity to faint background noise

    cv::Mat MaxVesselness = cv::Mat::zeros(Src.size(), CV_32FC1);

    // Search across different vein thicknesses
    for (float Sigma = SigmaStart; Sigma <= SigmaEnd; Sigma += SigmaStep) {
        cv::Mat Blurred;
        int     KSize = static_cast<int>(4 * Sigma + 1);
        if (KSize % 2 == 0)
            KSize++;
        cv::GaussianBlur(Src, Blurred, cv::Size(KSize, KSize), Sigma);

        // Measure how quickly brightness curves horizontally, vertically, and diagonally
        cv::Mat Dxx, Dyy, Dxy;
        cv::Sobel(Blurred, Dxx, CV_32F, 2, 0, 3);
        cv::Sobel(Blurred, Dyy, CV_32F, 0, 2, 3);
        cv::Sobel(Blurred, Dxy, CV_32F, 1, 1, 3);

        // Adjust measurements for the current thickness level
        Dxx *= (Sigma * Sigma);
        Dyy *= (Sigma * Sigma);
        Dxy *= (Sigma * Sigma);

        cv::Mat Vesselness = cv::Mat::zeros(Src.size(), CV_32FC1);
        for (int Y = 0; Y < Src.rows; Y++) {
            for (int X = 0; X < Src.cols; X++) {
                float Fxx = Dxx.at<float>(Y, X);
                float Fyy = Dyy.at<float>(Y, X);
                float Fxy = Dxy.at<float>(Y, X);

                // Calculate the 2D curvature shape at this pixel
                float Tmp = std::sqrt((Fxx - Fyy) * (Fxx - Fyy) + 4 * Fxy * Fxy);
                float L1  = (Fxx + Fyy - Tmp) / 2.0f;
                float L2  = (Fxx + Fyy + Tmp) / 2.0f;

                // Veins are dark valleys, so the crosswise curve must be positive
                if (L2 <= 0)
                    continue;

                // Check if it's an elongated line and has strong contrast
                float Rb = std::abs(L1) / std::abs(L2);
                float S  = std::sqrt(L1 * L1 + L2 * L2);

                float V = std::exp(-(Rb * Rb) / (2 * Beta * Beta)) *
                          (1.0f - std::exp(-(S * S) / (2 * C * C)));

                Vesselness.at<float>(Y, X) = V;
            }
        }

        // Keep the strongest vein response found across all thickness levels
        cv::max(MaxVesselness, Vesselness, MaxVesselness);
    }

    // Scale pixel brightness from 0 to 255
    cv::normalize(MaxVesselness, MaxVesselness, 0, 255, cv::NORM_MINMAX, CV_8U);
    return MaxVesselness;
}

/**
 * @brief Palm vein processing engine.
 *
 * Handles finding hand landmarks, extracting the palm square, highlighting vein lines,
 * converting them into compact 4-direction pattern templates, and matching users.
 */
class ZCore
{
  public:
    /**
     * @brief Preprocessing helper for full-size palm patches (512x512).
     *
     * Pads the edges, evens out dark and bright lighting, cleans up camera grain,
     * and runs the vein line enhancer.
     *
     * @param ROI         Input palm image.
     * @param TargetSize  Target image size in pixels (default: 512).
     * @return Cleaned image with vein lines highlighted.
     */
    static cv::Mat PreProcess(const cv::Mat& ROI, int TargetSize = 512)
    {
        cv::Mat GrayFrame = ROI.clone();

        // Add a temporary frame around the edges so filters don't darken the borders
        const int Padding = 10;
        cv::Mat   Padded;
        cv::copyMakeBorder(
            GrayFrame, Padded, Padding, Padding, Padding, Padding, cv::BORDER_REFLECT
        );

        // Boost faint local contrast across the palm
        auto Clahe = cv::createCLAHE(1.1, cv::Size(16, 16));
        Clahe->apply(Padded, Padded);

        // Smooth out speckle noise
        cv::medianBlur(Padded, Padded, 7);

        // Highlight vein lines
        cv::Mat Output = FrangiFilter(Padded, 3.5f, 3.5f + (7 * 0.5f), 0.5f);

        // Trim the temporary border back off
        Output = CropBorder(Output, Padding);

        cv::normalize(Output, Output, 0, 255, cv::NORM_MINMAX, CV_8U);

        return Output;
    }

    /**
     * @brief Finds the valley spaces between fingers.
     *
     * Separates the hand from the background.
     * Finds the center point of the palm.
     * Traces distance from the palm center to the hand outline.
     * The closest points along the outline are the finger valleys.
     *
     * @param Frame        Camera image of the hand.
     * @param SmoothSigma  Smooths outline bumps so tiny wrinkles aren't mistaken for valleys.
     * @param MinNeighbor  Spacing between valleys to prevent picking the same dip twice.
     * @return List of valley points in counter-clockwise order around the hand.
     */
    static std::vector<cv::Point2f>
    ValleyExRadial(const cv::Mat& Frame, float SmoothSigma = 5.0f, int MinNeighbor = 15);

    /**
     * @brief Calculates a stable square box over the palm using the finger valleys.
     *
     * Uses the valley between the index and middle fingers (P1) and the valley
     * between the ring and little fingers (P2) to find the angle of the hand.
     * Points the box downward into the center of the palm and rotates it with the hand
     * so users can hold their hand at any angle.
     *
     * @param P1           Valley between index and middle fingers.
     * @param P2           Valley between ring and little fingers.
     * @param Handedness   +1 for right hand, -1 for left hand (shifts box away from thumb).
     * @param ScaleFactor  Box size multiplier based on hand size (default: 2.8).
     * @return 4 corner points of the rotated palm box.
     */
    static std::vector<cv::Point2f>
    ROIGen(cv::Point2f P1, cv::Point2f P2, int Handedness = 1, float ScaleFactor = 2.8f);

    /**
     * @brief Draws green circles and number labels on detected finger valleys for the UI preview.
     *
     * @param Points     List of valley coordinates.
     * @param DestFrame  Image to draw on.
     */
    inline static void PointVisualizerEx(const std::vector<cv::Point2f>& Points, cv::Mat& DestFrame)
    {
        for (size_t I = 0; I < Points.size(); I++) {
            cv::circle(DestFrame, Points[I], 5, cv::Scalar(0, 255, 0), -1);

            cv::putText(
                DestFrame,
                std::to_string(I),
                Points[I] + cv::Point2f(5, -5),
                cv::FONT_HERSHEY_SIMPLEX,
                0.5,
                cv::Scalar(255, 255, 255),
                1
            );
        }
    }

    /**
     * @brief Draws the palm capture box outline on the camera preview.
     *
     * @param Points     4 corners of the palm box.
     * @param DestFrame  Image to draw on.
     */
    inline static void DrawROI(const std::vector<cv::Point2f>& Points, cv::Mat& DestFrame)
    {
        std::vector<std::vector<cv::Point>> IntPoints(1);
        for (const auto& P : Points) {
            IntPoints[0].push_back(cv::Point(static_cast<int>(P.x), static_cast<int>(P.y)));
        }

        cv::polylines(DestFrame, IntPoints, true, cv::Scalar(255), 2);
    }

    /**
     * @brief Crops and straightens the angled palm box into a flat square image.
     *
     * Even if the hand is tilted or closer/further from the camera, this warps
     * the palm region into a standardized upright square.
     *
     * @param Src        Camera frame.
     * @param ROIPoints  4 corners from ROIGen.
     * @param TargetSize Size of output square in pixels (default: 512).
     * @return Straightened, cropped palm image.
     */
    static inline cv::Mat ExtractPalmROI(
        const cv::Mat& Src, const std::vector<cv::Point2f>& ROIPoints, int TargetSize = 512
    )
    {
        if (ROIPoints.size() != 4)
            return cv::Mat();

        std::vector<cv::Point2f> DstPoints = {
            cv::Point2f(0, 0),
            cv::Point2f(static_cast<float>(TargetSize), 0),
            cv::Point2f(static_cast<float>(TargetSize), static_cast<float>(TargetSize)),
            cv::Point2f(0, static_cast<float>(TargetSize))
        };

        cv::Mat TransMat = cv::getPerspectiveTransform(ROIPoints, DstPoints);

        cv::Mat ROI;
        cv::warpPerspective(Src, ROI, TransMat, cv::Size(TargetSize, TargetSize), cv::INTER_CUBIC);

        return ROI;
    }

    /**
     * @brief Trims a small margin from around the image edges to remove blur and filter artifacts.
     *
     * @param Src         Input image.
     * @param BorderSize  Number of pixels to shave off each side.
     * @return Cropped image.
     */
    static cv::Mat CropBorder(const cv::Mat& Src, int BorderSize)
    {
        if (Src.empty())
            return Src;

        int SafeBorder = std::min({BorderSize, Src.cols / 2 - 1, Src.rows / 2 - 1});
        if (SafeBorder <= 0)
            return Src;

        cv::Rect InnerROI(
            SafeBorder, SafeBorder, Src.cols - (2 * SafeBorder), Src.rows - (2 * SafeBorder)
        );

        return Src(InnerROI).clone();
    }

    /**
     * @brief Measures how the image surface curves horizontally, vertically, and diagonally.
     *
     * Used to figure out which direction a vein line is running at each pixel.
     *
     * @param Src    Input image.
     * @param Lxx    Horizontal curvature map.
     * @param Lxy    Diagonal curvature map.
     * @param Lyy    Vertical curvature map.
     * @param Sigma  Smoothness scale for measurement.
     */
    static void
    CalculateHessian(const cv::Mat& Src, cv::Mat& Lxx, cv::Mat& Lxy, cv::Mat& Lyy, float Sigma);

    /**
     * @brief The main image enhancement pipeline for biometric scanning.
     *
     * Takes a cropped palm patch, scales it to 128x128, balances contrast,
     * removes grain, and extracts the vein lines and curvature maps.
     *
     * @param ROI  Cropped palm image.
     * @param Lxx  Output horizontal curvature map.
     * @param Lxy  Output diagonal curvature map.
     * @param Lyy  Output vertical curvature map.
     * @return Filtered image where veins glow bright white.
     */
    static cv::Mat PreProcessTest(const cv::Mat& ROI, cv::Mat& Lxx, cv::Mat& Lxy, cv::Mat& Lyy);

    /**
     * @brief Turns vein lines into a 4-direction pattern map (Horizontal, Vertical, Diagonals).
     *
     * For each pixel sitting on a vein, it checks which way the vein travels and tags it:
     *   - 0: Horizontal
     *   - 1: Diagonal Right
     *   - 2: Vertical
     *   - 3: Diagonal Left
     * Also produces a mask image so flat skin without veins is excluded from comparisons.
     *
     * @param Vesselness   Vein brightness map from PreProcessTest.
     * @param Lxx          Horizontal curvature map.
     * @param Lxy          Diagonal curvature map.
     * @param Lyy          Vertical curvature map.
     * @param OutTemplate  Output pattern map with values 0, 1, 2, or 3.
     * @param OutMask      Output mask (white = vein present, black = flat skin).
     */
    static void GenerateTemplate(
        const cv::Mat& Vesselness,
        const cv::Mat& Lxx,
        const cv::Mat& Lxy,
        const cv::Mat& Lyy,
        cv::Mat&       OutTemplate,
        cv::Mat&       OutMask
    );

    /**
     * @brief Compares two vein patterns to check if they belong to the same person.
     *
     * Slides one template over the other by a few pixels in every direction to handle
     * slight hand placement differences. Only compares pixels where both images actually
     * have veins:
     *   - Same direction:      0 penalty
     *   - 45 degree tilt:      0.5 penalty
     *   - 90 degree tilt:      1.0 penalty
     *
     * Requires at least 25% vein overlap so random noise cannot produce an accidental match.
     *
     * @param T1           Enrolled user's vein pattern.
     * @param M1           Enrolled user's vein mask.
     * @param T2           Live candidate's vein pattern.
     * @param M2           Live candidate's vein mask.
     * @param SearchRange  Pixel slide wiggle room (default: 5 -> +/- 5 pixels).
     * @return Score between 0.0 (exact match) and 1.0 (completely different). Matching threshold is
     * around 0.22.
     */
    static float Match(
        const cv::Mat& T1,
        const cv::Mat& M1,
        const cv::Mat& T2,
        const cv::Mat& M2,
        int            SearchRange = 5
    );

    /**
     * @brief Compresses a 128x128 vein pattern and mask into a compact 6 KB byte buffer.
     *
     * Stores 4 vein directions per byte (2 bits each) and 8 mask pixels per byte (1 bit each).
     * Makes saving to database or sending over network super small and fast.
     *
     * @param Orient  128x128 vein direction map (values 0..3).
     * @param Mask    128x128 vein mask (0 or 255).
     * @return Exactly 6,144 bytes (6 KB) of packed binary data.
     */
    static std::vector<uint8_t> PackTemplate(const cv::Mat& Orient, const cv::Mat& Mask);

    /**
     * @brief Restores the 6 KB binary data buffer back into full 128x128 template and mask images.
     *
     * @param Buffer  Packed 6 KB byte stream.
     * @param Orient  Output restored vein direction image.
     * @param Mask    Output restored vein mask image.
     * @param Width   Image width (default: 128).
     * @param Height  Image height (default: 128).
     */
    static void UnpackTemplate(
        const std::vector<uint8_t>& Buffer,
        cv::Mat&                    Orient,
        cv::Mat&                    Mask,
        int                         Width  = 128,
        int                         Height = 128
    );
};

#endif
