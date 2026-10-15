
#ifndef FILTRE_COMPLEMENTAIRE_H
#define FILTRE_COMPLEMENTAIRE_H

/*
 * Etat du filtre complémentaire.
 *
 * angle : dernière estimation de l'inclinaison (degrés)
 * tau   : constante de temps du filtre (secondes)
 */
typedef struct
{
    float angle;
    float tau;
} FiltreComplementaire;

/*
 * Initialise le filtre.
 *
 * angle_initial : angle de départ, mesuré par l'ACC
 * tau           : constante de temps en secondes
 */
void filtre_complementaire_init(
    FiltreComplementaire *filtre,
    float angle_initial,
    float tau
);

/*
 * Met à jour l'estimation de l'angle.
 *
 * vitesse_gyro : vitesse angulaire en deg/s
 * angle_acc    : angle mesuré par l'ACC en degrés
 * dt           : temps depuis la dernière mesure en secondes
 *
 * Retourne le nouvel angle filtré.
 */
float filtre_complementaire_update(
    FiltreComplementaire *filtre,
    float vitesse_gyro,
    float angle_acc,
    float dt
);

#endif
