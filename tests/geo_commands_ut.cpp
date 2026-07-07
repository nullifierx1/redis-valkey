#include "result_test_utils.hpp"

#include <database/database.hpp>

#include <gtest/gtest.h>

TEST(GeoCommands, GeoAddAddsAndUpdatesMembers) {
    DataBase db;

    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo"
    })), 0);
    ExpectString(db.Type("Sicily"), "geo");
}

TEST(GeoCommands, GeoAddRejectsInvalidCoordinatesAndWrongType) {
    DataBase db;

    ExpectError(
        db.Geoadd("bad", Args({"abc", "38.115556", "Palermo"})),
        "invalid longitude or latitude"
    );
    ExpectError(
        db.Geoadd("bad", Args({"13.361389", "90", "Palermo"})),
        "invalid longitude or latitude"
    );

    ExpectDone(db.Set("plain", "value"));
    ExpectError(
        db.Geoadd("plain", Args({"13.361389", "38.115556", "Palermo"})),
        "wrong type"
    );
}

TEST(GeoCommands, GeoAddWrongTypeWinsOverMaxMemoryLimit) {
    DataBase db;

    ExpectDone(db.Set("plain", "value"));
    ExpectDone(db.ConfigSetMaxMemory(1));

    ExpectError(
        db.Geoadd("plain", Args({"13.361389", "38.115556", "Palermo"})),
        "wrong type"
    );
    ExpectString(db.Get("plain"), "value");
}

TEST(GeoCommands, GeoPosReturnsCoordinatesForEachRequestedMember) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    const auto positions = ExpectGeoPositions(
        db.Geopos("Sicily", Args({"Palermo", "Missing", "Catania"})),
        3
    );

    ASSERT_TRUE(positions.values[0].has_value());
    EXPECT_EQ(positions.values[0]->longitude, "13.36");
    EXPECT_EQ(positions.values[0]->latitude, "38.12");
    EXPECT_FALSE(positions.values[1].has_value());
    ASSERT_TRUE(positions.values[2].has_value());
    EXPECT_EQ(positions.values[2]->longitude, "15.09");
    EXPECT_EQ(positions.values[2]->latitude, "37.50");
}

TEST(GeoCommands, GeoPosMissingKeyReturnsNilForEachRequestedMember) {
    DataBase db;

    const auto positions = ExpectGeoPositions(
        db.Geopos("missing", Args({"Palermo", "Catania"})),
        2
    );
    EXPECT_FALSE(positions.values[0].has_value());
    EXPECT_FALSE(positions.values[1].has_value());
}

TEST(GeoCommands, GeoDistMatchesValkeyWithinSmallMeterEpsilon) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    constexpr double kEpsilonMeters = 10.0;

    ExpectDistanceMetersNear(
        db.Geodist("Sicily", "Palermo", "Catania", "m"),
        166274.1516,
        kEpsilonMeters
    );
    ExpectDistanceMetersNear(
        db.Geodist("Sicily", "Palermo", "Catania", "km"),
        166274.1516,
        kEpsilonMeters,
        1000.0
    );
    ExpectDistanceMetersNear(
        db.Geodist("Sicily", "Palermo", "Catania", "mi"),
        166274.1516,
        kEpsilonMeters,
        1609.344
    );
    ExpectDistanceMetersNear(
        db.Geodist("Sicily", "Palermo", "Catania", "ft"),
        166274.1516,
        kEpsilonMeters,
        0.3048
    );
    ExpectNil(db.Geodist("Sicily", "Foo", "Bar", "m"));
}

TEST(GeoCommands, GeoDistReturnsZeroForSameMember) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo"
    })), 1);

    ExpectDistanceMetersNear(db.Geodist("Sicily", "Palermo", "Palermo", "m"), 0.0, 0.0001);
}

TEST(GeoCommands, GeoDistRejectsUnsupportedUnitAndWrongType) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    ExpectError(db.Geodist("Sicily", "Palermo", "Catania", "yard"), "unsupported unit");

    ExpectDone(db.Set("plain", "value"));
    ExpectError(db.Geodist("plain", "Palermo", "Catania", "m"), "wrong type");
}

TEST(GeoCommands, GeoSearchFindsMembersInsideRadius) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    ExpectArray(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "100", "km"})),
        {"Catania"}
    );
    ExpectArrayUnordered(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "200", "km"})),
        {"Palermo", "Catania"}
    );
}

TEST(GeoCommands, GeoSearchSortsAndLimitsResults) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    ExpectArray(
        db.Geosearch("Sicily", Args({
            "FROMLONLAT", "15", "37", "BYRADIUS", "200", "km", "ASC", "COUNT", "1"
        })),
        {"Catania"}
    );
    ExpectArray(
        db.Geosearch("Sicily", Args({
            "FROMLONLAT", "15", "37", "BYRADIUS", "200", "km", "DESC", "COUNT", "1"
        })),
        {"Palermo"}
    );
}

TEST(GeoCommands, GeoSearchRejectsInvalidValuesAndWrongType) {
    DataBase db;

    ExpectError(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "bad", "37", "BYRADIUS", "100", "km"})),
        "invalid longitude or latitude"
    );
    ExpectError(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "15", "90", "BYRADIUS", "100", "km"})),
        "invalid longitude or latitude"
    );
    ExpectError(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "-1", "km"})),
        "invalid radius"
    );
    ExpectError(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "100", "yard"})),
        "unsupported unit"
    );
    ExpectError(
        db.Geosearch("Sicily", Args({
            "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "COUNT", "0"
        })),
        "invalid count"
    );

    ExpectDone(db.Set("plain", "value"));
    ExpectError(
        db.Geosearch("plain", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "100", "km"})),
        "wrong type"
    );
}

TEST(GeoCommands, GeoSearchStoreSavesLimitedSearchResult) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    ExpectInteger(
        db.Geosearchstore("Nearby", "Sicily", Args({
            "FROMLONLAT", "15", "37", "BYRADIUS", "200", "km", "ASC", "COUNT", "1"
        })),
        1
    );
    ExpectArray(
        db.Geosearch("Nearby", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "200", "km"})),
        {"Catania"}
    );
}

TEST(GeoCommands, GeoSearchStoreCanOverwriteSourceKey) {
    DataBase db;
    ExpectInteger(db.Geoadd("Sicily", Args({
        "13.361389", "38.115556", "Palermo",
        "15.087269", "37.502669", "Catania"
    })), 2);

    ExpectInteger(
        db.Geosearchstore("Sicily", "Sicily", Args({
            "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km"
        })),
        1
    );
    ExpectArray(
        db.Geosearch("Sicily", Args({"FROMLONLAT", "15", "37", "BYRADIUS", "200", "km"})),
        {"Catania"}
    );
}

TEST(GeoCommands, GeoAddAllowsBoundaryCoordinates) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({
        "-180", "-85.05112878", "min",
        "180", "85.05112878", "max"
    })), 2);

    const auto positions = ExpectGeoPositions(db.Geopos("geo", Args({"min", "max"})), 2);
    ASSERT_TRUE(positions.values[0].has_value());
    ASSERT_TRUE(positions.values[1].has_value());
}

TEST(GeoCommands, GeoAddRejectsOutOfRangeLongitude) {
    DataBase db;

    ExpectError(db.Geoadd("geo", Args({"180.1", "0", "p"})), "invalid longitude or latitude");
}

TEST(GeoCommands, GeoAddInvalidCoordinatesDoNotCreateKey) {
    DataBase db;

    ExpectError(db.Geoadd("geo", Args({"999", "0", "p"})), "invalid longitude or latitude");
    ExpectString(db.Type("geo"), "none");
}

TEST(GeoCommands, GeoAddPartiallyInvalidBatchDoesNotCreateKey) {
    DataBase db;

    ExpectError(
        db.Geoadd("geo", Args({"10", "10", "ok", "999", "0", "bad"})),
        "invalid longitude or latitude"
    );
    ExpectString(db.Type("geo"), "none");
}

TEST(GeoCommands, GeoAddPartiallyInvalidBatchKeepsExistingValueUnchanged) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "old"})), 1);
    ExpectError(
        db.Geoadd("geo", Args({"11", "11", "new", "999", "0", "bad"})),
        "invalid longitude or latitude"
    );
    ExpectGeoPositions(db.Geopos("geo", Args({"old"})), 1);
    ExpectArray(db.Geosearch("geo", Args({"FROMLONLAT", "11", "11", "BYRADIUS", "1", "km"})), {});
}

TEST(GeoCommands, GeoPosWrongTypeReturnsError) {
    DataBase db;

    ExpectDone(db.Set("plain", "value"));
    ExpectError(db.Geopos("plain", Args({"p"})), "wrong type");
}

TEST(GeoCommands, GeoDistMissingKeyReturnsNil) {
    DataBase db;

    ExpectNil(db.Geodist("missing", "a", "b", "m"));
}

TEST(GeoCommands, GeoDistMissingOneMemberReturnsNil) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    ExpectNil(db.Geodist("geo", "a", "b", "m"));
}

TEST(GeoCommands, GeoDistUnitsAreCaseInsensitive) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a", "11", "11", "b"})), 2);
    ExpectDistanceMetersNear(db.Geodist("geo", "a", "b", "KM"), 155940.8917, 500.0, 1000.0);
}

TEST(GeoCommands, GeoSearchMissingKeyReturnsEmptyArray) {
    DataBase db;

    ExpectArray(
        db.Geosearch("missing", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "1", "km"})),
        {}
    );
}

TEST(GeoCommands, GeoSearchWithZeroRadiusFindsExactCenterMember) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a", "11", "11", "b"})), 2);
    ExpectArray(
        db.Geosearch("geo", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "0", "km"})),
        {"a"}
    );
}

TEST(GeoCommands, GeoSearchCountLargerThanMatchesReturnsAllMatches) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a", "11", "11", "b"})), 2);
    ExpectArrayUnordered(
        db.Geosearch("geo", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "200", "km", "COUNT", "10"})),
        {"a", "b"}
    );
}

TEST(GeoCommands, GeoSearchStoreMissingSourceReturnsZeroAndDoesNotCreateDestination) {
    DataBase db;

    ExpectInteger(
        db.Geosearchstore("dest", "missing", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "1", "km"})),
        0
    );
    ExpectString(db.Type("dest"), "none");
}

TEST(GeoCommands, GeoSearchStoreWrongTypeSourceReturnsError) {
    DataBase db;

    ExpectDone(db.Set("plain", "value"));
    ExpectError(
        db.Geosearchstore("dest", "plain", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "1", "km"})),
        "wrong type"
    );
    ExpectString(db.Type("dest"), "none");
}

TEST(GeoCommands, GeoSearchStoreWrongTypeDestinationReturnsErrorWithoutChangingDestination) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    ExpectDone(db.Set("dest", "value"));
    ExpectError(
        db.Geosearchstore("dest", "geo", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "1", "km"})),
        "wrong type"
    );
    ExpectString(db.Get("dest"), "value");
}

TEST(GeoCommands, GeoSearchStoreWrongTypeDestinationWinsOverMaxMemoryLimit) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    ExpectDone(db.Set("dest", "value"));
    ExpectDone(db.ConfigSetMaxMemory(1));

    ExpectError(
        db.Geosearchstore("dest", "geo", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "1", "km"})),
        "wrong type"
    );
    ExpectString(db.Get("dest"), "value");
}

TEST(GeoCommands, GeoSearchStoreWithNoMatchesCreatesEmptyGeoDestination) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    ExpectInteger(
        db.Geosearchstore("dest", "geo", Args({"FROMLONLAT", "0", "0", "BYRADIUS", "1", "km"})),
        0
    );
    ExpectString(db.Type("dest"), "geo");
    ExpectArray(db.Geosearch("dest", Args({"FROMLONLAT", "10", "10", "BYRADIUS", "1", "km"})), {});
}

TEST(GeoCommands, SetCommandOverwritesGeoValue) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    ExpectDone(db.Set("geo", "plain"));
    ExpectString(db.Type("geo"), "string");
    ExpectString(db.Get("geo"), "plain");
}

TEST(GeoCommands, DelRemovesGeoKey) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    ExpectInteger(db.Del(Args({"geo"})), 1);
    ExpectGeoPositions(db.Geopos("geo", Args({"a"})), 1);
    ExpectString(db.Type("geo"), "none");
}
