package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallBaseVisitor;
import hycallparser.antlr.HycallParser;

import java.math.BigInteger;

public class NumberVisitor extends HycallBaseVisitor<BigInteger> {

    @Override
    public BigInteger visitNumber_DECIMAL(HycallParser.Number_DECIMALContext ctx) {
        return new BigInteger(ctx.getText());
    }

    @Override
    public BigInteger visitNumber_HEXADEC(HycallParser.Number_HEXADECContext ctx) {
        return new BigInteger(ctx.getText().substring(2, ctx.getText().length()), 16);
    }

    @Override
    public BigInteger visitNumber_BINARY(HycallParser.Number_BINARYContext ctx) {
        return new BigInteger(ctx.getText().substring(2, ctx.getText().length()), 2);
    }
}
