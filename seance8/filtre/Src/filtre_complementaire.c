
#include "filtre_complementaire.h"

void filtre_complementaire_init(
    FiltreComplementaire *filtre,
    float angle_initial,
    float tau
)
{
    if (filtre == 0)
    {
        return;
    }

    filtre->angle = angle_initial;

    /*
     * Une constante de temps strictement positive
     * est nécessaire.
     */
    filtre->tau = (tau > 0.0f) ? tau : 0.083f;
}

float filtre_complementaire_update(
    FiltreComplementaire *filtre,
    float vitesse_gyro,
    float angle_acc,
    float dt
)
{
    if (filtre == 0 || dt <= 0.0f)
    {
        return 0.0f;
    }

    /*
     * 1. Prédiction par le gyroscope.
     */
    float prediction =
        filtre->angle + vitesse_gyro * dt;

    /*
     * 2. Calcul du coefficient à partir du temps.
     *
     * alpha = tau / (tau + dt)
     */
    float alpha = filtre->tau / (filtre->tau + dt);

    /*
     * 3. Correction avec l'accéléromètre.
     */
    filtre->angle =
        alpha * prediction
        + (1.0f - alpha) * angle_acc;

    return filtre->angle;
}
