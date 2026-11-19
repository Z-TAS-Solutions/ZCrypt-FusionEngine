#include "ZCore.h"

std::vector<cv::Point2f>
ZCore::ValleyExRadial(const cv::Mat& Frame, float SmoothSigma, int MinNeighbor)
{
    // Step 1: Make a black-and-white silhouette of the hand
    cv::Mat Gray, Mask, Dist;
    if (Frame.channels() == 3)
        cv::cvtColor(Frame, Gray, cv::COLOR_BGR2GRAY);
    else
        Gray = Frame;

    cv::GaussianBlur(Gray, Mask, cv::Size(7, 7), 0);
    cv::threshold(Mask, Mask, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    // Step 2: Find the center of the palm (the point farthest from any edge of the hand)
    cv::distanceTransform(Mask, Dist, cv::DIST_L2, 5);
    double    MaxVal;
    cv::Point MaxLoc;
    cv::minMaxLoc(Dist, nullptr, &MaxVal, nullptr, &MaxLoc);
    cv::Point2f Center = MaxLoc;

    // Step 3: Trace the outline of the hand
    std::vector<std::vector<cv::Point>> Contours;
    cv::findContours(Mask, Contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if (Contours.empty())
        return {};

    auto HandContour =
        *std::max_element(Contours.begin(), Contours.end(), [](const auto& A, const auto& B) {
            return cv::contourArea(A) < cv::contourArea(B);
        });

    // Scale smoothing window based on hand size
    float DynamicSigma    = static_cast<float>(MaxVal) * 0.12f;
    int   DynamicNeighbor = static_cast<int>(MaxVal * 0.25f);

    // Step 4: Measure distance from palm center to every point along the hand outline
    std::vector<float> RadialDistances;
    for (const auto& Pt : HandContour) {
        RadialDistances.push_back(
            static_cast<float>(cv::norm(Center - static_cast<cv::Point2f>(Pt)))
        );
    }

    // Step 5: Smooth out the distance measurements so small skin wrinkles don't look like valleys
    std::vector<float> Smoothed;
    cv::Mat            DistMat(RadialDistances, false);
    cv::GaussianBlur(DistMat, DistMat, cv::Size(1, 0), DynamicSigma);
    DistMat.copyTo(Smoothed);

    // Step 6: Find dips (closest points to palm center) which mark where fingers meet
    std::vector<cv::Point2f> Valleys;
    int                      N = static_cast<int>(Smoothed.size());
    for (int I = 0; I < N; ++I) {
        bool IsLocalMin = true;
        for (int J = -DynamicNeighbor; J <= DynamicNeighbor; ++J) {
            if (Smoothed[(I + J + N) % N] < Smoothed[I]) {
                IsLocalMin = false;
                break;
            }
        }

        if (IsLocalMin) {
            cv::Point2f Pt = HandContour[I];

            // Ignore points below the palm (that's the wrist / forearm, not fingers)
            if (Pt.y > Center.y + (MaxVal * 0.6f))
                continue;

            // Ignore points touching the camera screen edges
            if (Pt.x <= 10 || Pt.x >= Frame.cols - 11 || Pt.y <= 10)
                continue;

            // Ignore shallow false dips
            if (Smoothed[I] < (MaxVal * 0.9f))
                continue;

            Valleys.push_back(Pt);
        }
    }

    // Step 7: Order valleys counter-clockwise around the hand so we know which finger is which
    std::sort(Valleys.begin(), Valleys.end(), [&](const cv::Point2f& A, const cv::Point2f& B) {
        float AngA = std::atan2(A.y - Center.y, A.x - Center.x);
        float AngB = std::atan2(B.y - Center.y, B.x - Center.x);

        return AngA < AngB;
    });

    return Valleys;
}

std::vector<cv::Point2f>
ZCore::ROIGen(cv::Point2f P1, cv::Point2f P2, int Handedness, float ScaleFactor)
{
    // Distance between finger valleys (tells us how big the hand is)
    float Dx = P2.x - P1.x;
    float Dy = P2.y - P1.y;
    float D  = std::sqrt(Dx * Dx + Dy * Dy);

    // Center point between the two finger valleys
    cv::Point2f Mid = (P1 + P2) * 0.5f;

    // Unit directions that rotate with the hand:
    //   UnitDown:  Points inward toward the center of the palm
    //   UnitRight: Points sideways across the fingers
    cv::Point2f UnitDown(-Dy / D, Dx / D);
    cv::Point2f UnitRight(Dx / D, Dy / D);

    // Push the box center down into the middle of the palm
    float       OffsetDist = D * 2.2f;
    cv::Point2f ROICenter  = Mid + (UnitDown * OffsetDist);

    // Nudge box slightly away from the thumb side depending on left or right hand
    float ThumbShiftFactor = 0.3f;
    ROICenter              = ROICenter + (UnitRight * (D * ThumbShiftFactor * Handedness));

    // Calculate box radius
    float H = (D * ScaleFactor) / 2.0f;

    // Build the 4 rotated corners of the palm square
    std::vector<cv::Point2f> Corners(4);
    Corners[0] = ROICenter - (UnitRight * H) - (UnitDown * H); // Top-left
    Corners[1] = ROICenter + (UnitRight * H) - (UnitDown * H); // Top-right
    Corners[2] = ROICenter + (UnitRight * H) + (UnitDown * H); // Bottom-right
    Corners[3] = ROICenter - (UnitRight * H) + (UnitDown * H); // Bottom-left

    return Corners;
}

float ZCore::Match(
    const cv::Mat& T1, const cv::Mat& M1, const cv::Mat& T2, const cv::Mat& M2, int SearchRange
)
{
    // Check that both enrolled and live templates actually have data
    if (T1.empty() || M1.empty() || T2.empty() || M2.empty()) {
        return 1.0f; // No match
    }
    if (T1.size() != M1.size() || T2.size() != M2.size() || T1.size() != T2.size()) {
        return 1.0f;
    }

    int M1Count = cv::countNonZero(M1);
    int M2Count = cv::countNonZero(M2);

    // Require at least 25% overlapping vein pixels (min 500 px) to prevent noise false matches
    int MinRequiredOverlap = std::max(500, static_cast<int>(std::min(M1Count, M2Count) * 0.25f));

    float BestScore = 1.0f; // Lower score is a better match (0.0 = perfect match)

    // Slide live template slightly in X and Y (-SearchRange to +SearchRange) to find the best alignment
    for (int Dy = -SearchRange; Dy <= SearchRange; Dy++) {
        for (int Dx = -SearchRange; Dx <= SearchRange; Dx++) {

            float Total = 0.0f;
            float Diff  = 0.0f;

            // Only inspect pixels inside the boundary margin so shifting doesn't read out-of-bounds
            for (int I = SearchRange; I < T1.rows - SearchRange; I++) {
                for (int J = SearchRange; J < T1.cols - SearchRange; J++) {

                    // Only compare pixels where BOTH images confirm a vein is present
                    if (M1.at<uchar>(I, J) > 0 && M2.at<uchar>(I + Dy, J + Dx) > 0) {
                        Total += 1.0f;

                        int V1 = T1.at<uchar>(I, J);
                        int V2 = T2.at<uchar>(I + Dy, J + Dx);

                        if (V1 != V2) {
                            int Dist = std::abs(V1 - V2);
                            if (Dist == 3)
                                Dist = 1; // Direction 0 and 3 wrap around (only 45 degrees apart)

                            if (Dist == 1) {
                                Diff += 0.5f; // Small penalty for 45 degree tilt
                            } else {
                                Diff += 1.0f; // Full penalty for 90 degree perpendicular lines
                            }
                        }
                    }
                }
            }

            // Only score if there was enough overlapping vein area
            if (Total >= MinRequiredOverlap) {
                float CurrentScore = Diff / Total;
                if (CurrentScore < BestScore)
                    BestScore = CurrentScore;
            }
        }
    }
    return BestScore;
}

void ZCore::CalculateHessian(
    const cv::Mat& Src, cv::Mat& Lxx, cv::Mat& Lxy, cv::Mat& Lyy, float Sigma
)
{
    int KSize = static_cast<int>(4 * Sigma + 1);
    if (KSize % 2 == 0)
        KSize++;

    cv::Mat Blurred;
    cv::GaussianBlur(Src, Blurred, cv::Size(KSize, KSize), Sigma);

    // Calculate how brightness curves horizontally (Lxx), vertically (Lyy), and diagonally (Lxy)
    cv::Sobel(Blurred, Lxx, CV_32F, 2, 0, 3);
    cv::Sobel(Blurred, Lyy, CV_32F, 0, 2, 3);
    cv::Sobel(Blurred, Lxy, CV_32F, 1, 1, 3);
}

cv::Mat ZCore::PreProcessTest(const cv::Mat& ROI, cv::Mat& Lxx, cv::Mat& Lxy, cv::Mat& Lyy)
{
    cv::Mat Resized, Output;

    // Step 1: Resize palm image to standard 128x128 resolution
    cv::resize(ROI, Resized, cv::Size(128, 128), 0, 0, cv::INTER_AREA);

    // Step 2: Add mirror padding around edges so filters don't darken the borders
    const int Padding = 10;
    cv::Mat   Padded;
    cv::copyMakeBorder(Resized, Padded, Padding, Padding, Padding, Padding, cv::BORDER_REFLECT);

    // Step 3: Boost vein contrast against skin tone
    auto Clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    Clahe->apply(Padded, Padded);

    // Step 4: Clean up camera sensor grain
    cv::medianBlur(Padded, Padded, 3);

    // Step 5: Measure surface curvature directions
    float Sigma = 1.5f;
    CalculateHessian(Padded, Lxx, Lxy, Lyy, Sigma);

    // Step 6: Highlight vein tube lines across small and medium widths
    Output = FrangiFilter(Padded, 1.0f, 2.5f, 0.5f);

    // Step 7: Trim off the mirror padding to restore exact 128x128 size
    Output = ZCore::CropBorder(Output, Padding);
    Lxx    = ZCore::CropBorder(Lxx, Padding);
    Lxy    = ZCore::CropBorder(Lxy, Padding);
    Lyy    = ZCore::CropBorder(Lyy, Padding);

    cv::normalize(Output, Output, 0, 255, cv::NORM_MINMAX, CV_8U);

    return Output;
}

void ZCore::GenerateTemplate(
    const cv::Mat& Vesselness,
    const cv::Mat& Lxx,
    const cv::Mat& Lxy,
    const cv::Mat& Lyy,
    cv::Mat&       OutTemplate,
    cv::Mat&       OutMask
)
{
    OutTemplate = cv::Mat::zeros(Vesselness.size(), CV_8U);

    // Vein mask: Only pixels with high vein brightness are included in matching
    cv::threshold(Vesselness, OutMask, 50, 255, cv::THRESH_BINARY);

    for (int R = 0; R < Vesselness.rows; R++) {
        for (int C = 0; C < Vesselness.cols; C++) {
            if (OutMask.at<uchar>(R, C) == 0)
                continue; // Skip flat skin

            // Calculate the angle the vein line is pointing:
            float Angle =
                0.5f *
                std::atan2(2.0f * Lxy.at<float>(R, C), Lxx.at<float>(R, C) - Lyy.at<float>(R, C));

            float Deg = Angle * 180.0f / static_cast<float>(CV_PI);
            if (Deg < 0.0f)
                Deg += 180.0f;

            // Group the continuous angle into 4 simple compass directions:
            if (Deg < 22.5f || Deg >= 157.5f)
                OutTemplate.at<uchar>(R, C) = 0; // Horizontal (left-right)
            else if (Deg < 67.5f)
                OutTemplate.at<uchar>(R, C) = 1; // Diagonal Right
            else if (Deg < 112.5f)
                OutTemplate.at<uchar>(R, C) = 2; // Vertical (up-down)
            else
                OutTemplate.at<uchar>(R, C) = 3; // Diagonal Left
        }
    }
}

std::vector<uint8_t> ZCore::PackTemplate(const cv::Mat& Orient, const cv::Mat& Mask)
{
    std::vector<uint8_t> Buffer;
    Buffer.reserve(4096 + 2048);

    // Directions only need values 0, 1, 2, or 3 (2 bits each), so 4 pixels fit in 1 byte
    uint8_t CurrentByte = 0;
    int     BitCount    = 0;
    for (size_t I = 0; I < Orient.total(); ++I) {
        CurrentByte = (CurrentByte << 2) | (Orient.data[I] & 0x03);
        BitCount += 2;
        if (BitCount == 8) {
            Buffer.push_back(CurrentByte);
            CurrentByte = 0;
            BitCount    = 0;
        }
    }

    // Mask is binary (yes/no), so 8 pixels fit in 1 byte
    CurrentByte = 0;
    BitCount    = 0;
    for (size_t I = 0; I < Mask.total(); ++I) {
        CurrentByte = (CurrentByte << 1) | (Mask.data[I] > 0 ? 1 : 0);
        BitCount++;
        if (BitCount == 8) {
            Buffer.push_back(CurrentByte);
            CurrentByte = 0;
            BitCount    = 0;
        }
    }
    return Buffer; // Total size is exactly 6,144 bytes (6 KB)
}

void ZCore::UnpackTemplate(
    const std::vector<uint8_t>& Buffer,
    cv::Mat&                    Orient,
    cv::Mat&                    Mask,
    int                         Width,
    int                         Height
)
{
    int TotalPixels = Width * Height;
    Orient          = cv::Mat::zeros(Height, Width, CV_8U);
    Mask            = cv::Mat::zeros(Height, Width, CV_8U);

    size_t BufferIdx = 0;

    // Unpack 2 bits per pixel for vein direction
    int     BitsProcessed = 0;
    uint8_t CurrentByte   = 0;
    for (int I = 0; I < TotalPixels; ++I) {
        if (BitsProcessed == 0) {
            CurrentByte   = Buffer[BufferIdx++];
            BitsProcessed = 8;
        }
        Orient.data[I] = (CurrentByte >> (BitsProcessed - 2)) & 0x03;
        BitsProcessed -= 2;
    }

    // Unpack 1 bit per pixel for the vein mask
    BitsProcessed = 0;
    for (int I = 0; I < TotalPixels; ++I) {
        if (BitsProcessed == 0) {
            CurrentByte   = Buffer[BufferIdx++];
            BitsProcessed = 8;
        }
        Mask.data[I] = ((CurrentByte >> (BitsProcessed - 1)) & 0x01) ? 255 : 0;
        BitsProcessed -= 1;
    }
}
